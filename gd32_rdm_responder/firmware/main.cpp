/**
 * @file main.cpp
 *
 */
/* Copyright (C) 2021 by Arjan van Vught mailto:info@gd32-dmx.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <cstdio>
#include <cstdint>

#include "hardware.h"
#include "noemac/network.h"
#include "ledblink.h"

#include "displayudf.h"
#include "displayrdm.h"

#include "rdmresponder.h"
#include "rdmpersonality.h"

#include "rdmdeviceparams.h"
#include "rdmsensorsparams.h"
#include "rdmsubdevicesparams.h"

#include "identify.h"
#include "factorydefaults.h"

#include "ws28xxdmxparams.h"
#include "ws28xxdmx.h"

#include "flashrom.h"
#include "spiflashstore.h"
#include "storews28xxdmx.h"
#include "storerdmdevice.h"
#include "storerdmsensors.h"
#include "storerdmsubdevices.h"
#include "storedisplayudf.h"

#include "firmwareversion.h"
#include "software_version.h"

#include "dmx.h" //TODO rremove

void main(void) {
	Hardware hw;
	Network nw;
	LedBlink lb;
	DisplayUdf display;
	FirmwareVersion fw(SOFTWARE_VERSION, __DATE__, __TIME__);
	FlashRom flash;
	SpiFlashStore spiFlashStore;

	fw.Print();

	PixelDmxConfiguration pixelDmxConfiguration;

	StoreWS28xxDmx storeWS28xxDmx;
	WS28xxDmxParams ws28xxparms(&storeWS28xxDmx);

	if (ws28xxparms.Load()) {
		ws28xxparms.Set(&pixelDmxConfiguration);
		ws28xxparms.Dump();
	}

	pixelDmxConfiguration.SetCount(8); //TODO Remove pixelDmxConfiguration.SetCount(4)

	WS28xxDmx pixelDmx(pixelDmxConfiguration);
	DisplayRdm displayRdm;

	pixelDmx.SetLightSetDisplay(&displayRdm);
	pixelDmx.SetWS28xxDmxStore(&storeWS28xxDmx);

	const auto nCount = pixelDmxConfiguration.GetCount();
	char aDescription[32];
	snprintf(aDescription, sizeof(aDescription) -1, "%s:%d", PixelType::GetType(pixelDmxConfiguration.GetType()), nCount);

	RDMPersonality personality(aDescription, pixelDmx.GetDmxFootprint());

	Identify identify;

	StoreRDMSensors storeRdmSensors;
	RDMSensorsParams rdmSensorsParams(&storeRdmSensors);

	if (rdmSensorsParams.Load()) {
		rdmSensorsParams.Set();
		rdmSensorsParams.Dump();
	}

	StoreRDMSubDevices storeRdmSubDevices;
	RDMSubDevicesParams rdmSubDevicesParams(&storeRdmSubDevices);

	if (rdmSubDevicesParams.Load()) {
		rdmSubDevicesParams.Set();
		rdmSubDevicesParams.Dump();
	}

	RDMResponder rdmResponder(&personality, &pixelDmx);
	rdmResponder.Init();

	StoreRDMDevice storeRdmDevice;
	RDMDeviceParams rdmDeviceParams(&storeRdmDevice);

	if (rdmDeviceParams.Load()) {
		rdmDeviceParams.Set(&rdmResponder);
		rdmDeviceParams.Dump();
	}

	rdmResponder.SetRDMDeviceStore(&storeRdmDevice);

	FactoryDefaults factoryDefaults;
	rdmResponder.SetRDMFactoryDefaults(&factoryDefaults);

	rdmResponder.Print();

	hw.WatchdogInit();

	rdmResponder.SetOutput(&pixelDmx);
	rdmResponder.Start();

	pixelDmx.Print();

	display.SetTitle("RDM Responder Pixel 1");
	display.Set(2, displayudf::Labels::VERSION);
	display.Set(6, displayudf::Labels::DMX_START_ADDRESS);
	display.Printf(7, "%s:%d G%d",
			PixelType::GetType(pixelDmxConfiguration.GetType()),
			pixelDmxConfiguration.GetCount(),
			pixelDmxConfiguration.GetGroupingCount());

	StoreDisplayUdf storeDisplayUdf;
	DisplayUdfParams displayUdfParams(&storeDisplayUdf);

	if (displayUdfParams.Load()) {
		displayUdfParams.Set(&display);
		displayUdfParams.Dump();
	}

	display.Show();

	for(;;) {
		hw.WatchdogFeed();
		rdmResponder.Run();
		spiFlashStore.Flash();
		identify.Run();
		lb.Run();
	}
}
