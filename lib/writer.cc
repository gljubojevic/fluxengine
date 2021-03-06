#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "writer.h"
#include "sql.h"
#include "protocol.h"
#include "usb.h"
#include "dataspec.h"
#include "fmt/format.h"

static DataSpecFlag dest(
    { "--dest", "-d" },
    "destination for data",
    ":t=0-79:s=0-1");

static sqlite3* outdb;

void setWriterDefaultDest(const std::string& dest)
{
    ::dest.set(dest);
}

void writeTracks(
	const std::function<std::unique_ptr<Fluxmap>(int track, int side)> producer)
{
    const auto& spec = dest.value;

    std::cout << "Writing to: " << spec << std::endl;

	if (!spec.filename.empty())
	{
		outdb = sqlOpen(spec.filename, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
		sqlPrepareFlux(outdb);
		sqlStmt(outdb, "BEGIN;");
		atexit([]()
			{
				sqlStmt(outdb, "COMMIT;");
				sqlClose(outdb);
			}
		);
	}

    for (const auto& location : spec.locations)
    {
        std::cout << fmt::format("{0:>3}.{1}: ", location.track, location.side) << std::flush;
        std::unique_ptr<Fluxmap> fluxmap = producer(location.track, location.side);
        if (!fluxmap)
        {
            if (!outdb)
            {
                std::cout << "erasing" << std::endl;
                usbSeek(location.track);
                usbErase(location.side);
            }
        }
        else
        {
            fluxmap->precompensate(PRECOMPENSATION_THRESHOLD_TICKS, 2);
            if (outdb)
                sqlWriteFlux(outdb, location.track, location.side, *fluxmap);
            else
            {
                usbSeek(location.track);
                usbWrite(location.side, *fluxmap);
            }
            std::cout << fmt::format(
                "{0} ms in {1} bytes", int(fluxmap->duration()/1e6), fluxmap->bytes()) << std::endl;
        }
    }
}

void fillBitmapTo(std::vector<bool>& bitmap,
	unsigned& cursor, unsigned terminateAt,
	const std::vector<bool>& pattern)
{
	while (cursor < terminateAt)
	{
		for (bool b : pattern)
		{
			if (cursor < bitmap.size())
				bitmap[cursor++] = b;
		}
	}
}

