#include "globals.h"
#include "flags.h"
#include "dataspec.h"
#include "fluxreader.h"

static bool ends_with(const std::string& value, const std::string& ending)
{
    if (ending.size() > value.size())
        return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

std::unique_ptr<FluxReader> FluxReader::create(const DataSpec& spec)
{
    const auto& filename = spec.filename;

    if (filename.empty())
        return createHardwareFluxReader(spec.drive);
    else if (ends_with(filename, ".flux"))
        return createSqliteFluxReader(filename);
    else if (ends_with(filename, "/"))
        return createStreamFluxReader(filename);

    Error() << "unrecognised flux filename extension";
    return std::unique_ptr<FluxReader>();
}
