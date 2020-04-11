#pragma once
#include "mqusd.h"
#include "mqCommon/mqDummyPlugins.h"

namespace mqusd {

class mqabcExportPlugin;
class mqabcExportWindow;
class mqabcImportPlugin;
class mqabcImportWindow;
class mqabcRecorderPlugin;
class mqabcRecorderWindow;

void mqabcInitialize();
void mqabcFinalize();

} // namespace mqusd
