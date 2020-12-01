#include <iostream>
#include <vector>
#include <windows.h>
#include <stdint.h>
#include <math.h>
using namespace std;

#define VK_KEY_W		0x57
#define VK_KEY_D		0x44
#define VK_KEY_S		0x53
#define VK_KEY_A		0x41
#define VK_KEY_Q		0x51
#define VK_KEY_F		0x46

enum info {
	POINT_DISTANCE,
	ERROR_X,
	ERROR_Y,
	ADDR_H,
	ADDR_L,
	EVERYONE,
	SOMEBODY
};

typedef struct {
	DWORD address;
	float x, y, z;
	float px, py, pz;
	int8_t x_axis, y_axis;
	uint8_t direction;
	uint8_t SUPER_MODE :1 ;
	uint8_t F_MODE :1 ;
	uint8_t JUMP_MODE :1 ;
} self_t;

typedef struct {
	float current_x, current_y;
	float x, y;
} mouse_t;

typedef struct{
	void (*handler)();
	uint8_t key;
} keyhandler_t;

uint16_t loop_time = 5;
uint16_t T_10MS = 0;
self_t self;
mouse_t mouse;
HANDLE hProcess;

// ���o���a�Ҧ���T    
void GetPlayerInfo(void) {
	ReadProcessMemory(hProcess, (DWORD*)0x20538000, &self.address, 4, NULL);
	ReadProcessMemory(hProcess, (DWORD*)(self.address+0x18), &self.x, 4, NULL);
	ReadProcessMemory(hProcess, (DWORD*)(self.address+0x14), &self.y, 4, NULL);
	ReadProcessMemory(hProcess, (DWORD*)(self.address+0x1C), &self.z, 4, NULL);
	ReadProcessMemory(hProcess, (DWORD*)(self.address+0xB4), &mouse.current_x, 4, NULL);
	ReadProcessMemory(hProcess, (DWORD*)(self.address+0xB0), &mouse.current_y, 4, NULL);
    
    // �P�_��V
    if (mouse.current_x > 145 || mouse.current_x < -145) {
        self.y_axis = -1;
        self.x_axis = 0;
        self.direction = 0;
	}
    else if (mouse.current_x > 45 && mouse.current_x < 145) {
        self.x_axis = 1;
        self.y_axis = 0;
        self.direction = 1;
	}
    else if (mouse.current_x > -45 && mouse.current_x < 45) {
        self.y_axis = 1;
        self.x_axis = 0;
        self.direction = 2;
	}
    else if (mouse.current_x > -145 && mouse.current_x < -45) {
        self.x_axis = -1;
        self.y_axis = 0;
        self.direction = 3;
	}
}

// ���o���ļĤH�a�}���M��
vector<DWORD> GetTargetList(uint8_t person) {
    vector<DWORD> list;
	DWORD addr;
	DWORD base1 = 0x205565b0, base2 = 0x200C64f0;
	uint8_t id;
	uint8_t death;
	uint8_t hide;
	uint16_t appear1;
	uint16_t appear2;
	uint16_t act;
    
    // ��X���ĤH���a�}
    for (int i=0;i<100;i++) {
		ReadProcessMemory(hProcess, (DWORD*)base1, &addr, 4, NULL);
		ReadProcessMemory(hProcess, (DWORD*)(addr+0xA0), &id, 1, NULL);
        if (id == 0) {
            base1 = base1 - 0x588;
            base2 = base2 - 0x6C;
            continue;
		}
		if (person == EVERYONE) {
			list.push_back(base1);
		}
		else if (person == SOMEBODY) {
			ReadProcessMemory(hProcess, (DWORD*)(addr+0xBC), &death, 1, NULL);
			ReadProcessMemory(hProcess, (DWORD*)base2, &hide, 1, NULL);
			ReadProcessMemory(hProcess, (DWORD*)(addr+0x0C), &appear1, 2, NULL);
			ReadProcessMemory(hProcess, (DWORD*)(addr+0x10), &appear2, 2, NULL);
			ReadProcessMemory(hProcess, (DWORD*)(addr+0x50), &act, 2, NULL);
			/*
			if (base1 == 0x205456a8)
				printf("%d %d %d %d %d\n", death,act,hide,appear1,appear2);
			*/
		if (death != 0 && death <= 40 && appear1 < 100 && appear2 < 100 && hide == 0 &&\
				act > 0 && act <= 1022) {
				list.push_back(base1);
			}
		}
        base1 = base1 - 0x588;
        base2 = base2 - 0x6C;
	}

    return list;
}

void AutomaticAiming(void) {
	typedef struct target_t {
		DWORD address;
		float x, y, z;
	};
	vector<vector<float> > group;
	vector<DWORD> list;
	float dx, dy, dz;
	float angle, angle360;
	float mouse_cx, mouse_cy;
	float distance, z, tmp;
	float error_x,error_y;
	float sd;
	
	list = GetTargetList(SOMEBODY);
    if (list.size() > 0) {
        for (int target_index = 0; target_index < list.size(); target_index++) {
            GetPlayerInfo();
            // ���o�ĤH�Ҧ���T
			target_t target;
			ReadProcessMemory(hProcess, (DWORD*)list[target_index], &target.address, 4, NULL);
			ReadProcessMemory(hProcess, (DWORD*)(target.address + 0x18), &target.x, 4, NULL);
			ReadProcessMemory(hProcess, (DWORD*)(target.address + 0x14), &target.y, 4, NULL);
			ReadProcessMemory(hProcess, (DWORD*)(target.address + 0x1C), &target.z, 4, NULL);
			// �p��Ǥ߻P�ĤH����������
			dx =  self.x - target.x;
			dy = (self.y - target.y)*-1;
			
			distance = sqrt( pow(dx, 2) + pow(dy, 2) );
			// �Dx�b��������
			if (dx < 0 && dy == 0)
				angle360 = 0;
			else if (dx < 0 && dy < 0) // �ĤH�b�Ĥ@�H��
				angle360 = atan( fabs(dy/dx) ) * 180 / M_PI;
			else if (dx == 0 && dy < 0)
				angle360 = 90;
			else if (dx > 0 && dy < 0) // �ĤH�b�ĤG�H��
				angle360 = 90 + atan( fabs(dx/dy) ) * 180 / M_PI;
			else if (dx > 0 && dy == 0)
				angle360 = 180;
			else if (dx > 0 && dy > 0) // �ĤH�b�ĤT�H��
				angle360 = 180 + atan( fabs(dy/dx) ) * 180 / M_PI;
			else if (dx == 0 && dy > 0)
				angle360 = 270;
			else if (dx < 0 && dy > 0) // �ĤH�b�ĥ|�H��
				angle360 = 270 + atan( fabs(dx/dy) ) * 180 / M_PI;
			
			if (mouse.current_x < 0)
				mouse_cx = 360 + mouse.current_x - 90;
			else
				mouse_cx = mouse.current_x - 90; 
			mouse_cy = mouse.current_y * -1;
			
			angle = mouse_cx - angle360;
			if (angle360 - mouse_cx > 180)
				angle = 360 - angle360 + mouse_cx;
			else if (mouse_cx - angle360 > 180)
				angle = (360 + angle360 - mouse_cx)*-1;
			// �Dy�b��������
			tmp = self.z + 2;	// �ۨ����� + ������ (�˦a�O=39+25)
			z = tan( mouse_cy*M_PI/180 /* Deg2Rad */ ) * distance + tmp; // �D�ƹ�Y�b������� tan(deg) = ����/�F��
			dz = z - target.z;
			error_y = (atan(dz/distance) * 180 / M_PI)*-1;	// atan(����/�F��) * Deg2Rad
            error_x = angle;
			// end
            sd = sqrt( pow(error_x,2) + pow(error_y,2) );
            // �إߤH����T
			vector<float> character;
			character.push_back(sd);
			character.push_back(error_x);
			character.push_back(error_y);
			//character.push_back(target.x);
			//character.push_back(target.y);
			//character.push_back(target.z);
			// 32bit��Ƥ�2���O�s 
			character.push_back(target.address >> 16);			// high 16bit
			character.push_back(target.address &  0xffff);		// low 16bit
			// �[�J�M�椤
			group.push_back(character);
		}
        // �վ���������(�Z���Ѫ�컷)
        for (int index = 0; index < list.size(); index++) {
            for (int target_index = 0; target_index < list.size()-index-1; target_index++) {
                if ( group[target_index][POINT_DISTANCE] > group[target_index+1][POINT_DISTANCE] ) {
					vector<float> vec;	// �x�s�H����T
					vec.assign( group[target_index].begin(), group[target_index].end() );
					group[target_index].assign( group[target_index+1].begin(), group[target_index+1].end() );
					group[target_index+1].assign( vec.begin(), vec.end() );
				}
			}
		}
        // ���m�ĤH���W
		ReadProcessMemory(hProcess, (DWORD*)0x00F16A90, &mouse.x, 4, NULL);
		ReadProcessMemory(hProcess, (DWORD*)0x00F16A8C, &mouse.y, 4, NULL);
		if (fabs(group[0][ERROR_X]) < 90 && fabs(group[0][ERROR_Y]) < 87) {
			if (self.F_MODE == false) {
				mouse.x -= group[0][ERROR_X];
				mouse.y -= group[0][ERROR_Y];
				WriteProcessMemory(hProcess, (DWORD*)0x00F16A90, (LPVOID)&mouse.x, 4, NULL);
				WriteProcessMemory(hProcess, (DWORD*)0x00F16A8C, (LPVOID)&mouse.y, 4, NULL);
			} else {
				DWORD addr;
				float gtx, gty;
				
				addr = ((uint16_t)group[0][ADDR_H]*65536) + (uint16_t)group[0][ADDR_L];	// �B�zfloat�L�k�O�s4 byte���D 
				//cout << hex << ptr << endl;
				gtx = self.x+(50*self.x_axis);
				gty = self.y+(50*self.y_axis);
				WriteProcessMemory(hProcess, (DWORD*)(addr + 0x18), (LPVOID)&gtx, 4, NULL);
				WriteProcessMemory(hProcess, (DWORD*)(addr + 0x14), (LPVOID)&gty, 4, NULL);
				WriteProcessMemory(hProcess, (DWORD*)(addr + 0x1C), (LPVOID)&self.z, 4, NULL);
			}
		}
	}
}

void SavePlayerPosition(void) {
    self.px = self.x;
    self.py = self.y;
    self.pz = self.z;
}

void LoadPlayerPosition(void) {
	WriteProcessMemory(hProcess, (DWORD*)(self.address+0x18), (LPVOID)&self.px, 4, NULL);
	WriteProcessMemory(hProcess, (DWORD*)(self.address+0x14), (LPVOID)&self.py, 4, NULL);
	WriteProcessMemory(hProcess, (DWORD*)(self.address+0x1C), (LPVOID)&self.pz, 4, NULL);
}
    
void SuperJump(void) {
	int16_t h;
	
    if (self.JUMP_MODE == true) {
        for (int i=0;i<3;i++) {
			ReadProcessMemory(hProcess, (DWORD*)(self.address+0x2A), &h, 2, NULL);
			h += 50;
			WriteProcessMemory(hProcess, (DWORD*)(self.address+0x2A), (LPVOID)&h, 2, NULL);
		}
	}
}

void SuperPlayer(void) {
	uint8_t weapon;
	uint16_t shoot_time;
	uint16_t tmp16 = 0, num_byte;
	uint32_t tmp32 = 0;
	
	loop_time = 5;
	// Ū���ثe�Z��
	ReadProcessMemory(hProcess, (DWORD*)0x204F5B24, &weapon, 1, NULL);
	// �g���t��
	ReadProcessMemory(hProcess, (DWORD*)(self.address + 0x2C), &shoot_time, 2, NULL);
	if (shoot_time > 0 && weapon != 8)
        WriteProcessMemory(hProcess, (DWORD*)(self.address + 0x2C), (LPVOID)&tmp16, 2, NULL);
	// �j��L����
	if (shoot_time > 0 && (weapon == 18 || weapon == 8)) {
		WriteProcessMemory(hProcess, (DWORD*)(self.address + 0x4f0), (LPVOID)&tmp16, 2, NULL);
		WriteProcessMemory(hProcess, (DWORD*)(self.address + 0x4c8), (LPVOID)&tmp16, 2, NULL);
		loop_time = 6;
	}
    // ���}�����t�� 
	WriteProcessMemory(hProcess, (DWORD*)0x200C4735, (LPVOID)&tmp16, 1, NULL);
	// �L���l�u
	ReadProcessMemory(hProcess, (DWORD*)0x20033fc0, &num_byte, 2, NULL);
    if (num_byte != 0x9090) {
		num_byte = 0x9090;
        WriteProcessMemory(hProcess, (DWORD*)0x20033fc0, (LPVOID)&num_byte, 2, NULL);
	}
	// �L��y�O
	WriteProcessMemory(hProcess, (DWORD*)0x204F5F16, (LPVOID)&tmp32, 4, NULL);
	// ��O����
	tmp16 = 20000;
	WriteProcessMemory(hProcess, (DWORD*)(self.address + 0x478), (LPVOID)&tmp16, 2, NULL);
	// �}��L��y�O
	WriteProcessMemory(hProcess, (DWORD*)(self.address + 0x7E4), (LPVOID)&tmp32, 4, NULL);

	if (shoot_time > 0 && weapon == 7)
		loop_time = 25;
	// �۰ʧ���
	if (GetAsyncKeyState(0x2)) {
		if (weapon == 6 || weapon == 14 || weapon == 22) {
			mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);
			mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);
			loop_time = 50;
		}
	}
}       

void SuperRun(void) {
    uint8_t d = 80;
	int8_t gx = 0, gy = 0;
    int8_t gomap[4][8] = { { 0,-1,  1, 0,  0, 1, -1, 0},
						   { 1, 0,  0, 1, -1, 0,  0,-1},
						   { 0, 1, -1, 0,  0,-1,  1, 0},
						   {-1, 0,  0,-1,  1, 0,  0, 1} };
						   
    if (GetAsyncKeyState(VK_SHIFT) & GetAsyncKeyState(VK_KEY_W)) {
        gx = gomap[self.direction][0];
        gy = gomap[self.direction][1];
	}
	else if (GetAsyncKeyState(VK_SHIFT) & GetAsyncKeyState(VK_KEY_S)) {
        gx = gomap[self.direction][4];
        gy = gomap[self.direction][5];
	}
	// �p�G���U��Ӥ�V��h�ۥ[�ۨ��ƭ�
	// ex. ���UW 	gx = 1, gy = 0
	// �S���UD�h	gx = 0, gy = 1  
    if (GetAsyncKeyState(VK_SHIFT) & GetAsyncKeyState(VK_KEY_D)) {
        gx += gomap[self.direction][2];
        gy += gomap[self.direction][3];
	}
    else if (GetAsyncKeyState(VK_SHIFT) & GetAsyncKeyState(VK_KEY_A)) {
        gx += gomap[self.direction][6];
        gy += gomap[self.direction][7];
	}
	/*
    if (gx != 0 || gy != 0) {
    	float gtx, gty;
    	gtx = self.x+(d*gx);
    	gty = self.y+(d*gy);
		WriteProcessMemory(hProcess, (DWORD*)(self.address + 0x18), (LPVOID)&gtx, 4, NULL);
		WriteProcessMemory(hProcess, (DWORD*)(self.address + 0x14), (LPVOID)&gty, 4, NULL);
	}*/
	// �ק飼���t��  1���t�׬�127 
	if (gx != 0) {
		uint16_t ax;
		ReadProcessMemory(hProcess, (DWORD*)(self.address + 0x26), (LPVOID)&ax, 2, NULL);
		if (gx == -1) ax = 50048+127+32;
		else if (gx == 1) ax = 17280+127+32;
		WriteProcessMemory(hProcess, (DWORD*)(self.address + 0x26), (LPVOID)&ax, 2, NULL);
	}
	if (gy != 0) {
		uint16_t ay;
		ReadProcessMemory(hProcess, (DWORD*)(self.address + 0x22), (LPVOID)&ay, 2, NULL);
		if (gy == -1) ay = 50048+127+32;
		else if (gy == 1) ay = 17280+127+32;
		WriteProcessMemory(hProcess, (DWORD*)(self.address + 0x22), (LPVOID)&ay, 2, NULL);
	}
}

void MoveTargetPosition(void) {
    vector<DWORD> list;
	DWORD addr;
	float gtx, gty;
	
    list = GetTargetList(EVERYONE);
    for (int i=0; i<list.size(); i++) {
		ReadProcessMemory(hProcess, (DWORD*)list[i], (LPVOID)&addr, 4, NULL);
		gtx = self.x+(100*self.x_axis);
		gty = self.y+(100*self.y_axis);
		WriteProcessMemory(hProcess, (DWORD*)(addr + 0x18), (LPVOID)&gtx, 4, NULL);
		WriteProcessMemory(hProcess, (DWORD*)(addr + 0x14), (LPVOID)&gty, 4, NULL);
		WriteProcessMemory(hProcess, (DWORD*)(addr + 0x1C), (LPVOID)&self.z, 4, NULL);
	}
}

uint8_t getkey(void) {
	/* �����u���ץѤW��U */
    if (GetAsyncKeyState(VK_KEY_F))
        return VK_KEY_F;
    else if (GetAsyncKeyState(VK_SPACE))
        return VK_SPACE;
    else if (GetAsyncKeyState(VK_SHIFT))
        return VK_SHIFT;
    else if (GetAsyncKeyState(VK_KEY_Q))
        return VK_KEY_Q;
    else if (GetAsyncKeyState(VK_F1))
        return VK_F1;
    else if (GetAsyncKeyState(VK_F2))
        return VK_F2;
    else if (GetAsyncKeyState(VK_F3))
        return VK_F3;
    else if (GetAsyncKeyState(VK_F4))
        return VK_F4;
    else if (GetAsyncKeyState(VK_F5))
        return VK_F5;
    else if (GetAsyncKeyState(VK_F6))
        return VK_F6;
    else if (GetAsyncKeyState(VK_F7))
        return VK_F7;
    else if (GetAsyncKeyState(VK_F8))
        return VK_F8;
    return 0;
}

keyhandler_t key_map[] = 
{
	SuperJump, 			VK_SPACE,
	AutomaticAiming, 	VK_KEY_F,
	MoveTargetPosition,	VK_F1,
	SavePlayerPosition, VK_F5,
	LoadPlayerPosition, VK_F6,
	NULL,				0
};

int main()
{
	HWND hwnd;
	DWORD pid;
	
	DWORD addr;
	uint16_t act;
	uint8_t vKey, key;
	uint16_t tmp;
	// ���ݹC���}��
	while (1) {
		hwnd = FindWindow(NULL, "Wolfenstein");
		if (hwnd == 0) {
			cout << "             \r";
			cout << "�ж}�ҹC��\r";
			Sleep(300);
			cout << "�ж}�ҹC��.\r";
			Sleep(300);
			cout << "�ж}�ҹC��..\r";
			Sleep(300);
			cout << "�ж}�ҹC��...\r";
			Sleep(300);
		} else
			break;
	}
	GetWindowThreadProcessId(hwnd, &pid);
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
	
	cout << "Shift+��V��    �e�]" << endl;
    cout << "Space           �W�H��" << endl;
    cout << "F               �ۺ�/�l�H" << endl;
    cout << "Q               �����ۺ�/�l�H" << endl;
    cout << "F1              ���ϧl�H" << endl;
    cout << "F2              �}�ҶW�H�Ҧ�" << endl;
    cout << "F3              �����W�H�Ҧ�" << endl;
    cout << "F5              �]�m�O���I" << endl;
    cout << "F6              �^��O���I" << endl;
    cout << "F8              Exit" << endl;
    cout << "-----------------------------" << endl;
    cout << "�W�H�Ҧ���O:\t�ֳt����" << endl;
	cout << "\t\t�W�Ÿ��D" << endl;
	cout << "\t\t��O����" << endl;
	cout << "\t\t�L��y�O" << endl;
	cout << "\t\t�l�u����" << endl;
	while (1)
	{
		vKey = getkey();
		if (vKey > 0) {
			GetPlayerInfo();
			SuperRun();
			switch (vKey) {
				case VK_KEY_Q:
					if (self.F_MODE == true) {
						self.F_MODE = false;
						PlaySound("off.wav", NULL, SND_FILENAME);
					}
					else if (self.F_MODE == false) {
						self.F_MODE = true;
						PlaySound("on.wav", NULL, SND_FILENAME);
					}
					break;
				case VK_F2:
					self.SUPER_MODE = true;
					tmp = 0x9090;
					WriteProcessMemory(hProcess, (DWORD*)0x20033fc0, (LPVOID)&tmp, 2, NULL);
					PlaySound("on.wav", NULL, SND_FILENAME);
					break;
				case VK_F3:
					self.SUPER_MODE = false;
					tmp = 0x1029;
					WriteProcessMemory(hProcess, (DWORD*)0x20033fc0, (LPVOID)&tmp, 2, NULL);
					PlaySound("off.wav", NULL, SND_FILENAME);
					break;
			}/*
			for (int i = 0; i < KEYMAP_SIZE; i++) {
				if (key_map[i].key == key) {
					key_map[i].handler();
					if (key != VK_SPACE && key != VK_KEY_F) PlaySound("Success.wav", NULL, SND_FILENAME);
					break;
				}
			}*/
			if (vKey == VK_F8) {
				PlaySound("Success.wav", NULL, SND_FILENAME);
				break;
			}
			for ( keyhandler_t *keyhandler = key_map;
				  key = keyhandler->key;
				  keyhandler++ ) {
				if (vKey == key) {
					keyhandler->handler();
					if (vKey != VK_SPACE && vKey != VK_KEY_F) PlaySound("Success.wav", NULL, SND_FILENAME);
					break;
				}
			}
		}
		if (T_10MS >= loop_time && self.SUPER_MODE == true) {
			SuperPlayer();
			T_10MS = 0;
		}
		// Ū�����A
		if (self.SUPER_MODE == true) {
			ReadProcessMemory(hProcess, (DWORD*)(self.address + 0x50), &act, 2, NULL);
			if (act > 0 && act <= 1022)
				self.JUMP_MODE = true;
			else
				self.JUMP_MODE = false;
		}
		T_10MS++;
		Sleep(10);	// ����`���ӧ� 
	}
	
	return 0;
}
