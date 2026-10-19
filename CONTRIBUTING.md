# Contributing

Bug reports and fixes are welcome. For player reports, include the relevant settings, location, expected behavior, and what happened. For travel issues, include the origin, destination, and whether you traveled, restarted, or retried.

Never post server passwords, private room data, or personal save directories in a public issue. Review logs before attaching them.

See [BUILDING.md](docs/BUILDING.md) for the source layout and build commands. Keep changes focused and describe how they were tested. Preserve item and location IDs when updating a released world.

Gameplay hooks, save handling, and world transitions need both offline checks and targeted live tests. Keep a recoverable copy of the last working implementation before changing them. Use isolated development saves; do not adopt or modify a player's vanilla saves. Do not use broad process-memory scans or run multiple competing writers.

Do not automatically launch or close a player's game. Keep installation and cleanup recoverable, and preserve existing files on an unexpected mismatch.

Original contributions are covered by the repository's MIT license. Third-party code and artwork require their own attribution and applicable permissions; do not label game assets as MIT-licensed.
