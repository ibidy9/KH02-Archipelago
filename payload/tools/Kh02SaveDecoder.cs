// Read-only Steam KH02 PNG decoder. No save-writing or encryption API.
// Constants and derivation recovered from the supported executable's save/load
// wrappers, not from the unrelated generic PcSaveArchive format.
using System;
using System.IO;
using System.Text;
using System.Security.Cryptography;

public static class Kh02SaveDecoder
{
    private const string Seed = "u&nDPi-Mi5qpMkm)fcT4vqHYDTMh&t8z";
    private const string Alphabet = "ABCDE!#$%&FGHIJ012345KLMNOPqrstuvwxyzQRSTUVWXYZ6789abcdefgh},.<>ijklmnop()=~|-^+*;:[]{/?_@";
    public const int RawLength = 0x1725B0;
    public const int SlotOffset = 0xB0;
    public const int SlotSize = 0x3B40;

    private static uint BigEndian(byte[] data, int offset)
    {
        return ((uint)data[offset] << 24) | ((uint)data[offset + 1] << 16)
            | ((uint)data[offset + 2] << 8) | data[offset + 3];
    }

    private static uint Crc(byte[] data, int offset, int length)
    {
        uint crc = 0xffffffff;
        for (int i = offset; i < offset + length; i++)
        {
            crc ^= data[i];
            for (int bit = 0; bit < 8; bit++)
                crc = (crc >> 1) ^ ((crc & 1) != 0 ? 0xedb88320u : 0);
        }
        return ~crc;
    }

    public static byte[] Decode(byte[] png, string steamId)
    {
        if (steamId == null || steamId.Length != 17)
            throw new InvalidDataException("Expected the explicit 17-digit Steam profile ID");
        foreach (char c in steamId)
            if (c < '0' || c > '9') throw new InvalidDataException("Invalid Steam profile ID");
        byte[] signature = {137, 80, 78, 71, 13, 10, 26, 10};
        if (png == null || png.Length < 20 || png.Length > 16 * 1024 * 1024)
            throw new InvalidDataException("Invalid PNG size");
        for (int i = 0; i < 8; i++)
            if (png[i] != signature[i]) throw new InvalidDataException("Invalid PNG signature");
        byte[] payload = null;
        int offset = 8;
        bool ended = false;
        while (offset < png.Length)
        {
            if (png.Length - offset < 12) throw new InvalidDataException("Truncated PNG chunk");
            uint size = BigEndian(png, offset);
            if (size > (uint)(png.Length - offset - 12)) throw new InvalidDataException("Invalid PNG chunk size");
            int length = (int)size;
            string type = Encoding.ASCII.GetString(png, offset + 4, 4);
            if (Crc(png, offset + 4, length + 4) != BigEndian(png, offset + length + 8))
                throw new InvalidDataException("PNG CRC mismatch: " + type);
            if (offset == 8 && (type != "IHDR" || length != 13))
                throw new InvalidDataException("Missing initial IHDR");
            if (type == "sqEX")
            {
                if (payload != null) throw new InvalidDataException("Duplicate save payload");
                payload = new byte[length];
                Buffer.BlockCopy(png, offset + 8, payload, 0, length);
            }
            offset += length + 12;
            if (type == "IEND")
            {
                if (length != 0 || offset != png.Length) throw new InvalidDataException("Invalid PNG end");
                ended = true;
                break;
            }
        }
        if (!ended || payload == null || payload.Length != RawLength + 17
            || payload[RawLength] != 0)
            throw new InvalidDataException("Unsupported KH02 Steam payload layout");

        string identity = steamId + "1";
        byte[] key = new byte[32];
        for (int i = 0; i < key.Length; i++)
            key[i] = (byte)Alphabet[(Seed[i] ^ identity[(i + 1) % identity.Length]) % 90];
        byte[] decoded;
        using (Aes aes = Aes.Create())
        {
            aes.Key = key;
            aes.Mode = CipherMode.ECB;
            aes.Padding = PaddingMode.None;
            using (ICryptoTransform transform = aes.CreateDecryptor())
                decoded = transform.TransformFinalBlock(payload, 0, RawLength);
        }
        using (MD5 md5 = MD5.Create())
        {
            byte[] digest = md5.ComputeHash(decoded);
            int mismatch = 0;
            for (int i = 0; i < 16; i++) mismatch |= digest[i] ^ payload[RawLength + 1 + i];
            if (mismatch != 0) throw new InvalidDataException("Save digest mismatch: incorrect identity or corrupt payload");
        }
        if (Encoding.ASCII.GetString(decoded, 0, 4) != "SAVE")
            throw new InvalidDataException("Decoded data is not a raw KH02 SAVE");
        return decoded;
    }
}
