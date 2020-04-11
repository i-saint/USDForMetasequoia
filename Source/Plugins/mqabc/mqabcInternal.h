#pragma once
#include "mqusd.h"
#include "mqCommon/mqDummyPlugins.h"

namespace mqusd {

class mqabcExporterPlugin;
class mqabcExporterWindow;
class mqabcImporterPlugin;
class mqabcImporterWindow;
class mqabcRecorderPlugin;
class mqabcRecorderWindow;

void mqabcInitialize();
void mqabcFinalize();

} // namespace mqusd
