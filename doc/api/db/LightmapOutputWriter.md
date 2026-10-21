# Lightmap output ownership

The former db::LightmapOutputWriter and its public header are removed.
Use [bake::LightmapOutputWriter](../bake/LightmapOutputWriter.md) with the
[standalone batch](../bake/StandaloneBakeBatch.md). It publishes complete unique
generations and an authoritative final manifest, with typed native float output,
protected source dependencies and checked model-copy snapshots.

Generic [file IO](FileIO.md), [image encoding](ImageFormat.md) and native texture
archive IO remain in db. There is no forwarding alias or legacy flat-file writer.
