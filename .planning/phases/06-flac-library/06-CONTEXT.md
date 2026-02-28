# Phase 6: FLAC Library - Context

**Gathered:** 2026-02-28
**Status:** Ready for planning

<domain>
## Phase Boundary

Directory scanning, SQLite-backed library database, hierarchical browse models, and playlist playback for local FLAC files. Users can browse by Artist > Album > Track and play any track or album with next/previous navigation. QML UI integration is Phase 10's responsibility — this phase provides the data models and controllers.

</domain>

<decisions>
## Implementation Decisions

### Scanning behavior
- Automatic incremental scan on app startup (unchanged files skipped via mtime per FLAC-04)
- Emit scanProgress(current, total) signal for optional UI binding — Phase 10 decides how to display it
- Library is browsable during an active scan — models show whatever is in the DB, new entries appear as they're indexed
- Medium library size expected (~1,000-5,000 albums) — incremental scanning keeps subsequent startups fast

### Browse experience
- Artists: sorted alphabetically by name, each row shows artist name + album count
- Albums within an artist: sorted by year (oldest first — chronological discography order)
- Album row info: thumbnail album art + album title + year + track count
- Track row info: track number + title + per-track artist + duration (handles compilations/various artists)
- Multi-disc albums: flat list ordered by disc number then track number (no disc headers/separators in model)

### Playlist & playback
- Selecting a track queues the whole album as the playlist, starting at the selected track
- Next/previous navigates within the album (disc-aware ordering)
- Album finishes → stop playback (no looping, no continuation to next album)
- No shuffle — always sequential by disc then track number
- Playback is always user-initiated (consistent with CD-04 pattern)

### Album art
- Extract front + back covers from FLAC picture blocks (consistent with CdAlbumArtInfo pattern)
- Fallback: scan album directory for cover.jpg, folder.jpg, front.jpg (and back variants)
- Final fallback: generic placeholder
- Art extraction happens during library scan (not on-demand) — art is ready when user browses
- Cached in dedicated directory (~/.cache/media-console/album-art/) with SHA-1(artist+album) filenames per FLAC-05
- Never modify the user's music library directory

### Claude's Discretion
- SQLite schema design and query optimization
- QtConcurrent threading strategy for async scan
- Exact TagLib metadata extraction approach
- libsndfile + libsamplerate integration details for FlacAudioStream
- How browse models refresh during active scan (signal strategy)
- Error handling for corrupt/unreadable FLAC files

</decisions>

<specifics>
## Specific Ideas

No specific requirements — open to standard approaches. Follow established patterns from the CD subsystem (CdController, CdMetadataCache, CdAlbumArtProvider).

</specifics>

<code_context>
## Existing Code Insights

### Reusable Assets
- `AudioStream` interface (src/audio/AudioStream.h): FlacAudioStream implements this — outputs 44100Hz/16-bit/stereo
- `LocalPlaybackController` (src/audio/LocalPlaybackController.h): Handles play/pause/stop/seek for any AudioStream. Shared with CD.
- `PlaybackState` (src/state/PlaybackState.h): Has trackNumber/trackCount properties for navigation, plus title/artist/album/albumArtUrl
- `CdMetadataCache` (src/cd/CdMetadataCache.h): SQLite pattern with open/close/lookup/store — library DB follows same approach
- `CdAlbumArtProvider` (src/cd/CdAlbumArtProvider.h): Art extraction/caching pattern — library art provider follows same approach
- `CdController` (src/cd/CdController.h): Lifecycle orchestrator pattern — FlacLibraryController follows similar structure
- `AppConfig::LibraryConfig` (src/app/AppConfig.h): Already exists with rootPath = "/data/media/music/"

### Established Patterns
- Composition root: AppBuilder constructs all objects, passes non-owning pointers via AppContext
- State objects are thin Q_PROPERTY bags — no business logic
- SQLite for persistent data (CdMetadataCache pattern)
- QtConcurrent for async operations (FLAC-06)
- Signals/slots for async communication between components
- shared_ptr<AudioStream> passed to LocalPlaybackController::play()

### Integration Points
- FlacLibraryController wired into AppBuilder with LocalPlaybackController* and PlaybackState*
- Three QAbstractListModel subclasses registered for QML binding (Phase 10)
- LibraryConfig from AppConfig provides rootPath
- Album art paths set on PlaybackState::albumArtUrl during playback

</code_context>

<deferred>
## Deferred Ideas

- Shuffle playback mode — could be added as Phase 10 UI toggle
- Continue to next album after current finishes — potential future enhancement
- File system watcher for live library updates — startup scan is sufficient for now

</deferred>

---

*Phase: 06-flac-library*
*Context gathered: 2026-02-28*
