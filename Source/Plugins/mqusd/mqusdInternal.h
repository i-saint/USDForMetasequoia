#pragma once
#include "mqusd.h"
#include "mqCommon/mqDummyPlugins.h"

namespace mqusd {

class mqusdExportPlugin;
class mqusdExportWindow;
class mqusdImportPlugin;
class mqusdImportWindow;
class mqusdRecorderPlugin;
class mqusdRecorderWindow;

void mqusdInitialize();
void mqusdFinalize();

} // namespace mqusd
