#include <lib/driver/rcinput.h>

#include <lib/base/eerror.h>

#include <sys/ioctl.h>
#include <linux/input.h>
#include <sys/stat.h>

#include <lib/base/ebase.h>
#include <lib/base/init.h>
#include <lib/base/init_num.h>
#include <lib/driver/input_fake.h>

#ifdef VUPLUS_USE_RCKBD	
#include <lib/driver/rcconsole.h>
extern eRCConsole* g_ConsoleDevice;
#define CODE_RC    0
#define CODE_ASCII 1

#define KEY_LALT 	56
#define KEY_RALT 	100
#define KEY_LSHIFT 	42
static int special_key_mode = 0;

int getType(int code)
{
	switch(code)
	{
	case 2:   // 1
	case 3:   // 2
	case 4:   // 3
	case 5:   // 4
	case 6:   // 5
	case 7:   // 6
	case 8:   // 7
	case 9:   // 8
	case 10:  // 9
	case 11:  // 0
	case 14:  // backspace
	case 102: // home
	case 103: // up
	case 105: // left
	case 106: // right
	case 107: // end
	case 108: // down
	case 113: // mute
	case 114: // volume down
	case 115: // vulume up
	case 116: // power
	case 128: // stop
	case 138: // help
	case 139: // menu
	case 163: // FF
	case 208: // FF
	case 164: // pause
	case 165: // RW
	case 168: // RW
	case 167: // record
	case 174: // exit
	case 207: // play
	case 352: // ok
	case 358: // epg
	case 370: // _subtitle selection
	case 377: // tv
	case 384: // tape
	case 385: // radio
	case 388: // =text
	case 392: // audio
	case 393: // =recorded files
	case 398: // red
	case 399: // green
	case 400: // yellow
	case 401: // blue
	case 402: // channel up
	case 403: // channel down
	case 407: // >
	case 412: // <
		return CODE_RC;
	}
	return CODE_ASCII;
}
#endif /*VUPLUS_USE_RCKBD*/

void eRCDeviceInputDev::handleCode(long rccode)
{
	struct input_event *ev = (struct input_event *)rccode;
	if (ev->type!=EV_KEY)
		return;
#ifdef VUPLUS_USE_RCKBD
	//eDebug("value : %d, code : %d, type : %d, type : %s", ev->value, ev->code, ev->type, getType(ev->code)?"ASCII":"RC");
	if(getType(ev->code) || special_key_mode)
	{
		switch(ev->value)
		{
			case 0:
				if(ev->code == KEY_RALT || ev->code == KEY_LSHIFT || ev->code == KEY_LALT)
				{
					special_key_mode = 0;
					g_ConsoleDevice->handleCode(0);
				}
				break;
			case 1: 
				if(ev->code == KEY_RALT || ev->code == KEY_LSHIFT || ev->code == KEY_LALT)
					special_key_mode = 1;
				g_ConsoleDevice->handleCode(ev->code);
				break;
			case 2: break;
		}
		return;
	}
#endif /*VUPLUS_USE_RCKBD*/

//	eDebug("%x %x %x", ev->value, ev->code, ev->type);
	int km = iskeyboard ? input->getKeyboardMode() : eRCInput::kmNone;

//	eDebug("keyboard mode %d", km);
	
	if (km == eRCInput::kmAll)
		return;

	if (km == eRCInput::kmAscii)
	{
//		eDebug("filtering.. %d", ev->code);
		bool filtered = ( ev->code > 0 && ev->code < 61 );
		switch (ev->code)
		{
			case KEY_RESERVED:
			case KEY_ESC:
			case KEY_TAB:
			case KEY_BACKSPACE:
			case KEY_ENTER:
			case KEY_LEFTCTRL:
			case KEY_RIGHTSHIFT:
			case KEY_LEFTALT:
			case KEY_CAPSLOCK:
			case KEY_INSERT:
			case KEY_DELETE:
			case KEY_MUTE:
				filtered=false;
			default:
				break;
		}
		if (filtered)
			return;
//		eDebug("passed!");
	}

	switch (ev->value)
	{
	case 0:
		/*emit*/ input->keyPressed(eRCKey(this, ev->code, eRCKey::flagBreak));
		break;
	case 1:
		/*emit*/ input->keyPressed(eRCKey(this, ev->code, 0));
		break;
	case 2:
		/*emit*/ input->keyPressed(eRCKey(this, ev->code, eRCKey::flagRepeat));
		break;
	}
}

eRCDeviceInputDev::eRCDeviceInputDev(eRCInputEventDriver *driver)
	:eRCDevice(driver->getDeviceName(), driver), iskeyboard(false)
{
	if (strcasestr(id.c_str(), "keyboard") != NULL)
		iskeyboard = true;
	setExclusive(true);
	eDebug("Input device \"%s\" is %sa keyboard.", id.c_str(), iskeyboard ? "" : "not ");
}

void eRCDeviceInputDev::setExclusive(bool b)
{
	if (!iskeyboard)
		driver->setExclusive(b);
}

const char *eRCDeviceInputDev::getDescription() const
{
	return id.c_str();
}

class eInputDeviceInit
{
	struct element
	{
		public:
			char *m_filename;
			eRCInputEventDriver *m_driver;
			eRCDeviceInputDev *m_device;

			element(const char *filename, eRCInputEventDriver *driver, eRCDeviceInputDev *device):
				m_filename(strdup(filename)), m_driver(driver), m_device(device)
			{
			}
			~element()
			{
				delete m_device;
				delete m_driver;
				free(m_filename);
			}
	};
	typedef std::vector<element*> itemlist;
	itemlist m_items;
public:
	eInputDeviceInit()
	{
		int i = 0;
		while (1)
		{
			struct stat s;
			char filename[128];
			sprintf(filename, "/dev/input/event%d", i);
			if (stat(filename, &s))
				break;

			add(filename);
			++i;
		}
		eDebug("Found %d input devices!", i);
	}
	
	~eInputDeviceInit()
	{
		for (itemlist::iterator it = m_items.begin(); it != m_items.end(); ++it)
		{
			delete (*it);
		}
	}

	void add(const char* filename)
	{
		bool no_exist = false;
		for (itemlist::iterator it = m_items.begin(); it != m_items.end(); ++it)
		{
			if (strcmp((*it)->m_filename, filename) == 0)
			{
				eDebug("[eInputDeviceInit] %s is already added.", filename);
				no_exist = true;
				break;
			}
		}

		if (!no_exist)
		{
			eDebug("[eInputDeviceInit] adding device %s", filename);
			eRCInputEventDriver *driver = new eRCInputEventDriver(filename);
			eRCDeviceInputDev *device = new eRCDeviceInputDev(driver);
			m_items.push_back(new element(filename, driver, device));
		}
	}

	void remove(const char* filename)
	{
		for (itemlist::iterator it = m_items.begin(); it != m_items.end(); ++it)
		{
			if (strcmp((*it)->m_filename, filename) == 0)
			{
				eDebug("[eInputDeviceInit] remove device %s", filename);

				delete *it;
				m_items.erase(it);
				return;
			}
		}
		eDebug("[eInputDeviceInit] Remove '%s', not found", filename);
	}
};

eAutoInitP0<eInputDeviceInit> init_rcinputdev(eAutoInitNumbers::rc+1, "input device driver");

void addInputDevice(const char* filename)
{
	init_rcinputdev->add(filename);
}

void removeInputDevice(const char* filename)
{
	init_rcinputdev->remove(filename);
}

