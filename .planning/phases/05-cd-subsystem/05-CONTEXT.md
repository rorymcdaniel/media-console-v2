# Phase 5: CD Subsystem - Context

**Gathered:** 2026-02-28
**Status:** Ready for planning

<domain>
## Phase Boundary

Users can insert a CD, see track listing immediately, watch metadata fill in progressively, and play any track with paranoia error correction. Covers: CdAudioStream (libcdio/paranoia), TOC reading, disc presence detection, three-tier async metadata lookup (MusicBrainz → GnuDB → Discogs), SQLite metadata cache, album art downloading/caching, idle spindle management, and spin-up handling. Does NOT include FLAC playback, GPIO hardware, Spotify, or QML UI layout — those are separate phases.

</domain>

<decisions>
## Implementation Decisions

### Metadata cascade & progressive display
- Sequential fallback: MusicBrainz first, then GnuDB if MusicBrainz fails, then Discogs if GnuDB fails — stop at first success
- Initial display shows track numbers + durations only (no placeholder text like "Unknown Album")
- When metadata arrives, all track titles swap in instantly (batch update, no animation)
- If all three sources fail, silently show "Audio CD" as artist/album — no error indicators or "metadata unavailable" hints

### Album art sourcing & presentation
- CoverArtArchive first (tied to MusicBrainz disc ID), Discogs as fallback only if CoverArtArchive has no art
- Album art fades in over ~300ms when downloaded — smooth transition even though text uses instant swap
- Front and back cover support as per requirements

### Idle & spin-up timing
- 5-minute idle timer before spindle stop — configurable via `CdConfig::idleTimeoutSeconds` (default 300)
- During spin-up delay, play button shows loading/spinner state — playback starts automatically once spun up
- No toast or separate dialog for spin-up — the button state is sufficient feedback

### Disc detection & removal behavior
- Immediate "Reading disc..." loading state when insertion is detected — appears before TOC is read
- Disc removal during playback: immediate stop, silent clear of track listing — no toast or confirmation
- Reinsertion of previously-seen disc: re-read TOC, match disc ID against SQLite cache, show full metadata instantly if found (no network lookup)
- Non-audio disc insertion: brief toast notification ("Not an audio disc") — then nothing else

### Claude's Discretion
- Front/back cover interaction pattern (tap to flip, swipe carousel, etc.)
- No-art placeholder visual (generic CD icon, styled text, etc.)
- Spin-up timeout duration before showing an error
- Loading skeleton/spinner design for "Reading disc..." state
- Exact debounce timing for hybrid disc detection (QFileSystemWatcher + ioctl polling)
- GnuDB response validation specifics
- SQLite cache schema design

</decisions>

<specifics>
## Specific Ideas

- Track listing should feel immediate — the "Reading disc..." state bridges the gap so users never see a blank screen after inserting a disc
- Metadata fill-in should feel like a single "pop" — not a trickle of partial data
- The system should feel smart about known discs — reinserting a disc you played yesterday should feel instant
- Non-audio disc handling is polite but minimal — one toast, then silence

</specifics>

<code_context>
## Existing Code Insights

### Reusable Assets
- `ICdDrive` interface (`src/platform/ICdDrive.h`): Already defines openDevice, readToc (returns `QVector<TocEntry>`), getDiscId, eject, stopSpindle, isDiscPresent, isAudioDisc, trackCount
- `TocEntry` struct: trackNumber, startSector, endSector, durationSeconds — ready for track listing display
- `AudioStream` base class (`src/audio/AudioStream.h`): CdAudioStream implements this — open, readFrames, seek, totalFrames, positionFrames
- `LocalPlaybackController` (`src/audio/LocalPlaybackController.h`): Takes `shared_ptr<AudioStream>` via play() — ready to accept CdAudioStream
- `StubCdDrive` (`src/platform/stubs/StubCdDrive.h`): Test stub with setDiscPresent() for unit testing
- `AudioRingBuffer` + `AudioBufferStats`: 8-second buffer, 1-second prefill — already integrated in LocalPlaybackController
- `CdConfig` in `AppConfig`: devicePath ("/dev/cdrom"), pollIntervalMs (1000), audioOnly (true) — add idleTimeoutSeconds here

### Established Patterns
- Platform abstraction via interfaces (ICdDrive) with PlatformFactory providing real/stub implementations
- State layer as thin Q_PROPERTY bags (ReceiverState, PlaybackState, UIState) registered as QML singletons
- AppBuilder composition root with AppContext for dependency wiring
- Background thread playback with atomic flag control (stop, pause, seek pending)
- Logging categories defined (media.cd would be the new one)

### Integration Points
- `AppContext::cdDrive` pointer — already wired, needs real CdDrive implementation from PlatformFactory
- `LocalPlaybackController::play(shared_ptr<AudioStream>)` — CdAudioStream plugs in here
- `PlaybackState` — position/duration/track info updates during CD playback
- `PlatformFactory` — needs to create real CdDrive (libcdio) on Pi, StubCdDrive elsewhere
- New components needed: CdAudioStream, CdMetadataProvider (async lookup), CdMetadataCache (SQLite), CdController (orchestrates detection/metadata/playback lifecycle)

</code_context>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 05-cd-subsystem*
*Context gathered: 2026-02-28*
