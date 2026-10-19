from __future__ import annotations


OBJECTIVE_LOCATION_ID_BASE = 0x020280


def completed_objective_numbers(objective_bytes: bytes) -> set[int]:
    if len(objective_bytes) != 7:
        raise ValueError("KH 0.2 objective completion bitset must be 7 bytes")
    return {
        number
        for number in range(1, 52)
        if objective_bytes[(number - 1) // 8] & (1 << ((number - 1) % 8))
    }


def objective_location_id(number: int) -> int:
    if not 1 <= number <= 51:
        raise ValueError("KH 0.2 objective number must be between 1 and 51")
    return OBJECTIVE_LOCATION_ID_BASE + number


def completed_objective_location_ids(objective_bytes: bytes) -> set[int]:
    return {objective_location_id(number) for number in completed_objective_numbers(objective_bytes)}
