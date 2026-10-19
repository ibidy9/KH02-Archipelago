param(
    [Parameter(Mandatory=$true)][string]$InputPath,
    [Parameter(Mandatory=$true)][string]$SteamId,
    [Parameter(Mandatory=$true)][string]$OutputDirectory
)
$ErrorActionPreference='Stop'
Add-Type -Path (Join-Path $PSScriptRoot 'Kh02SaveDecoder.cs')
$source=(Resolve-Path -LiteralPath $InputPath).Path
$destination=[IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $destination) { throw 'Output directory must be new; never overwrite an existing capture.' }
$bytes=[IO.File]::ReadAllBytes($source)
$decoded=[Kh02SaveDecoder]::Decode($bytes,$SteamId)
function HashBytes([byte[]]$data) {
    $hasher=[Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($hasher.ComputeHash($data))).Replace('-','') }
    finally { $hasher.Dispose() }
}
$slots=@()
for($i=0;$i -lt 100;$i++) {
    $offset=[Kh02SaveDecoder]::SlotOffset+$i*[Kh02SaveDecoder]::SlotSize
    $slot=New-Object byte[] ([Kh02SaveDecoder]::SlotSize)
    [Array]::Copy($decoded,$offset,$slot,0,$slot.Length)
    $map=[Text.Encoding]::ASCII.GetString($slot,0x3324,0x100).Split([char]0)[0]
    $slots+=@{index=$i;sha256=(HashBytes $slot);flags=[int]$slot[0];level=[int]$slot[0x20];
        story=[BitConverter]::ToInt32($slot,0x2e10);map=$map;
        potion=[BitConverter]::ToInt32($slot,0xb04);hi_potion=[BitConverter]::ToInt32($slot,0xb0c);
        ether=[BitConverter]::ToInt32($slot,0xb1c)}
}
$report=@{format='kh02-steam-sqEX-aes256ecb-v1';input_sha256=(HashBytes $bytes);
    raw_sha256=(HashBytes $decoded);raw_length=$decoded.Length;png_crcs_verified=$true;
    native_digest_verified=$true;slots=$slots}
[IO.Directory]::CreateDirectory($destination) | Out-Null
# CreateNew also refuses an unexpected file appearing after the directory check.
$stream=[IO.File]::Open((Join-Path $destination 'decoded.raw'),[IO.FileMode]::CreateNew)
try {$stream.Write($decoded,0,$decoded.Length)} finally {$stream.Dispose()}
$json=$report | ConvertTo-Json -Depth 6
$stream=[IO.File]::Open((Join-Path $destination 'report.json'),[IO.FileMode]::CreateNew)
$writer=New-Object IO.StreamWriter($stream)
try {$writer.WriteLine($json)} finally {$writer.Dispose()}
if ((HashBytes ([IO.File]::ReadAllBytes($source))) -ne $report.input_sha256) {throw 'Input changed during decode; reject this capture.'}
Write-Output "Verified read-only decode saved to $destination"
