#include "imuread.h"


void print_data(const char *name, const unsigned char *data, int len)
{
	int i;

	printf("%s (%2d bytes):", name, len);
	for (i=0; i < len; i++) {
		printf(" %02X", data[i]);
	}
	printf("\n");
}

static int packet_primary_data(const unsigned char *data)
{
	//current_position.x = (float)((int16_t)((data[13] << 8) | data[12])) / 10.0f;
	//current_position.y = (float)((int16_t)((data[15] << 8) | data[14])) / 10.0f;
	//current_position.z = (float)((int16_t)((data[17] << 8) | data[16])) / 10.0f;
	current_orientation.q0 = (float)((int16_t)((data[25] << 8) | data[24])) / 30000.0f;
	current_orientation.q1 = (float)((int16_t)((data[27] << 8) | data[26])) / 30000.0f;
	current_orientation.q2 = (float)((int16_t)((data[29] << 8) | data[28])) / 30000.0f;
	current_orientation.q3 = (float)((int16_t)((data[31] << 8) | data[30])) / 30000.0f;
#if 0
	printf("mag data, %5.2f %5.2f %5.2f\n",
		current_position.x,
		current_position.y,
		current_position.z
	);
#endif
#if 0
	printf("orientation: %5.3f %5.3f %5.3f %5.3f\n",
		current_orientation.w,
		current_orientation.x,
		current_orientation.y,
		current_orientation.z
	);
#endif
	return 1;
}

static int packet_magnetic_cal(const unsigned char *data)
{
	int16_t id, x, y, z;
	int n;

	id = (data[7] << 8) | data[6];
	x = (data[9] << 8) | data[8];
	y = (data[11] << 8) | data[10];
	z = (data[13] << 8) | data[12];

	if (id == 1) {
		magcal.V[0] = (float)x * 0.1f;
		magcal.V[1] = (float)y * 0.1f;
		magcal.V[2] = (float)z * 0.1f;
		return 1;
	} else if (id == 2) {
		magcal.invW[0][0] = (float)x * 0.001f;
		magcal.invW[1][1] = (float)y * 0.001f;
		magcal.invW[2][2] = (float)z * 0.001f;
		return 1;
	} else if (id == 3) {
		magcal.invW[0][1] = (float)x / 1000.0f;
		magcal.invW[1][0] = (float)x / 1000.0f; // TODO: check this assignment
		magcal.invW[0][2] = (float)y / 1000.0f;
		magcal.invW[1][2] = (float)y / 1000.0f; // TODO: check this assignment
		magcal.invW[1][2] = (float)z / 1000.0f;
		magcal.invW[2][1] = (float)z / 1000.0f; // TODO: check this assignment
		return 1;
	} else if (id >= 10 && id < MAGBUFFSIZE+10) {
		n = id - 10;
		if (magcal.valid[n] == 0 || x != magcal.BpFast[0][n]
		  || y != magcal.BpFast[1][n] || z != magcal.BpFast[2][n]) {
			magcal.BpFast[0][n] = x;
			magcal.BpFast[1][n] = y;
			magcal.BpFast[2][n] = z;
			magcal.valid[n] = 1;
			//printf("mag cal, n=%3d: %5d %5d %5d\n", n, x, y, z);
		}
		return 1;
	}
	return 0;
}

static int packet(const unsigned char *data, int len)
{
	if (len <= 0) return 0;
	//print_data("packet", data, len);

	if (data[0] == 1 && len == 34) {
		return packet_primary_data(data);
	} else if (data[0] == 6 && len == 14) {
		return packet_magnetic_cal(data);
	}
	return 0;
}

static int packet_encoded(const unsigned char *data, int len)
{
	const unsigned char *p;
	unsigned char buf[256];
	int buflen=0, copylen;

	//printf("packet_encoded, len = %d\n", len);
	p = reinterpret_cast<const unsigned char *>(memchr(data, 0x7D, len));
	if (p == NULL) {
		return packet(data, len);
	} else {
		//printf("** decoding necessary\n");
		while (1) {
			copylen = p - data;
			if (copylen > 0) {
				//printf("  copylen = %d\n", copylen);
				if (buflen + copylen > int(sizeof(buf))) return 0;
				memcpy(buf+buflen, data, copylen);
				buflen += copylen;
				data += copylen;
				len -= copylen;
			}
			if (buflen + 1 > int(sizeof(buf))) return 0;
			buf[buflen++] = (p[1] == 0x5E) ? 0x7E : 0x7D;
			data += 2;
			len -= 2;
			if (len <= 0) break;
			p = reinterpret_cast<const unsigned char *>(memchr(data, 0x7D, len));
			if (p == NULL) {
				if (buflen + len > int(sizeof(buf))) return 0;
				memcpy(buf+buflen, data, len);
				buflen += len;
				break;
			}
		}
		//printf("** decoded to %d\n", buflen);
		return packet(buf, buflen);
	}
}

static int packet_parse(const unsigned char *data, int len)
{
	static unsigned char packetbuf[256];
	static unsigned int packetlen=0;
	const unsigned char *p;
	int copylen;
	int ret=0;

	//print_data("packet_parse", data, len);
	while (len > 0) {
		p = reinterpret_cast<const unsigned char *>(memchr(data, 0x7E, len));
		if (p == NULL) {
			if (packetlen + len > sizeof(packetbuf)) {
				packetlen = 0;
				return 0;  // would overflow buffer
			}
			memcpy(packetbuf+packetlen, data, len);
			packetlen += len;
			len = 0;
		} else if (p > data) {
			copylen = p - data;
			if (packetlen + copylen > sizeof(packetbuf)) {
				packetlen = 0;
				return 0;  // would overflow buffer
			}
			memcpy(packetbuf+packetlen, data, copylen);
			packet_encoded(packetbuf, packetlen+copylen);
			packetlen = 0;
			data += copylen + 1;
			len -= copylen + 1;
		} else {
			if (packetlen > 0) {
				if (packet_encoded(packetbuf, packetlen)) ret = 1;
				packetlen = 0;
			}
			data++;
			len--;
		}
	}
	return ret;
}

#define ASCII_STATE_WORD  0
#define ASCII_STATE_RAW   1
#define ASCII_STATE_CAL1  2
#define ASCII_STATE_CAL2  3

static int ascii_parse(const unsigned char *data, int len)
{
	static int ascii_state=ASCII_STATE_WORD;
	static int ascii_num=0, ascii_neg=0, ascii_count=0;
	static int16_t ascii_raw_data[9];
	static float ascii_cal_data[10];
	static unsigned int ascii_raw_data_count=0;
	const char *p, *end;
	int ret=0;

	//print_data("ascii_parse", data, len);
	end = (const char *)(data + len);
	for (p = (const char *)data ; p < end; p++) {
		if (ascii_state == ASCII_STATE_WORD) {
			if (ascii_count == 0) {
				if (*p == 'R') {
					ascii_num = ASCII_STATE_RAW;
					ascii_count = 1;
				} else if (*p == 'C') {
					ascii_num = ASCII_STATE_CAL1;
					ascii_count = 1;
				}
			} else if (ascii_count == 1) {
				if (*p == 'a') {
					ascii_count = 2;
				} else {
					ascii_num = 0;
					ascii_count = 0;
				}
			} else if (ascii_count == 2) {
				if (*p == 'w' && ascii_num == ASCII_STATE_RAW) {
					ascii_count = 3;
				} else if (*p == 'l' && ascii_num == ASCII_STATE_CAL1) {
					ascii_count = 3;
				} else {
					ascii_num = 0;
					ascii_count = 0;
				}
			} else if (ascii_count == 3) {
				if (*p == ':' && ascii_num == ASCII_STATE_RAW) {
					ascii_state = ASCII_STATE_RAW;
					ascii_raw_data_count = 0;
					ascii_num = 0;
					ascii_count = 0;
				} else if (*p == '1' && ascii_num == ASCII_STATE_CAL1) {
					ascii_count = 4;
				} else if (*p == '2' && ascii_num == ASCII_STATE_CAL1) {
					ascii_num = ASCII_STATE_CAL2;
					ascii_count = 4;
				} else {
					ascii_num = 0;
					ascii_count = 0;
				}
			} else if (ascii_count == 4) {
				if (*p == ':' && ascii_num == ASCII_STATE_CAL1) {
					ascii_state = ASCII_STATE_CAL1;
					ascii_raw_data_count = 0;
					ascii_num = 0;
					ascii_count = 0;
				} else if (*p == ':' && ascii_num == ASCII_STATE_CAL2) {
					ascii_state = ASCII_STATE_CAL2;
					ascii_raw_data_count = 0;
					ascii_num = 0;
					ascii_count = 0;
				} else {
					ascii_num = 0;
					ascii_count = 0;
				}
			} else {
				goto fail;
			}
		} else if (ascii_state == ASCII_STATE_RAW) {
			if (*p == '-') {
				//printf("ascii_parse negative\n");
				if (ascii_count > 0) goto fail;
				ascii_neg = 1;
			} else if (isdigit(*p)) {
				//printf("ascii_parse digit\n");
				ascii_num = ascii_num * 10 + *p - '0';
				ascii_count++;
			} else if (*p == ',') {
				//printf("ascii_parse comma, %d\n", ascii_num);
				if (ascii_neg) ascii_num = -ascii_num;
				if (((int16_t)ascii_num) != ascii_num) goto fail;
				if (ascii_raw_data_count >= 8) goto fail;
				ascii_raw_data[ascii_raw_data_count++] = ascii_num;
				ascii_num = 0;
				ascii_neg = 0;
				ascii_count = 0;
			} else if (*p == 13) {
				//printf("ascii_parse newline\n");
				if (ascii_neg) ascii_num = -ascii_num;
				if (((int16_t)ascii_num) != ascii_num) goto fail;
				if (ascii_raw_data_count != 8) goto fail;
				ascii_raw_data[ascii_raw_data_count] = ascii_num;
				raw_data(ascii_raw_data);
				ret = 1;
				ascii_raw_data_count = 0;
				ascii_num = 0;
				ascii_neg = 0;
				ascii_count = 0;
				ascii_state = ASCII_STATE_WORD;
			} else if (*p == 10) {
			} else {
				goto fail;
			}
		} else if (ascii_state == ASCII_STATE_CAL1 || ascii_state == ASCII_STATE_CAL2) {
			if (*p == '-') {
				//printf("ascii_parse negative\n");
				if (ascii_count > 0) goto fail;
				ascii_neg = 1;
			} else if (isdigit(*p)) {
				//printf("ascii_parse digit\n");
				ascii_num = ascii_num * 10 + *p - '0';
				ascii_count++;
			} else if (*p == '.') {
				//printf("ascii_parse decimal, %d\n", ascii_num);
				if (ascii_raw_data_count > 9) goto fail;
				ascii_cal_data[ascii_raw_data_count] = (float)ascii_num;
				ascii_num = 0;
				ascii_count = 0;
			} else if (*p == ',') {
				//printf("ascii_parse comma, %d\n", ascii_num);
				if (ascii_raw_data_count > 9) goto fail;
				ascii_cal_data[ascii_raw_data_count] +=
					(float)ascii_num / powf(10.0f, ascii_count);
				if (ascii_neg) ascii_cal_data[ascii_raw_data_count] *= -1.0f;
				ascii_raw_data_count++;
				ascii_num = 0;
				ascii_neg = 0;
				ascii_count = 0;
			} else if (*p == 13) {
				//printf("ascii_parse newline\n");
				if ((ascii_state == ASCII_STATE_CAL1 && ascii_raw_data_count != 9)
				 || (ascii_state == ASCII_STATE_CAL2 && ascii_raw_data_count != 8))
					goto fail;
				ascii_cal_data[ascii_raw_data_count] +=
					(float)ascii_num / powf(10.0f, ascii_count);
				if (ascii_neg) ascii_cal_data[ascii_raw_data_count] *= -1.0f;
				if (ascii_state == ASCII_STATE_CAL1) {
					cal1_data(ascii_cal_data);
				} else if (ascii_state == ASCII_STATE_CAL2) {
					cal2_data(ascii_cal_data);
				}
				ret = 1;
				ascii_raw_data_count = 0;
				ascii_num = 0;
				ascii_neg = 0;
				ascii_count = 0;
				ascii_state = ASCII_STATE_WORD;
			} else if (*p == 10) {
			} else {
				goto fail;
			}
		}
	}
	return ret;
fail:
	//printf("ascii FAIL\n");
	ascii_state = ASCII_STATE_WORD;
	ascii_raw_data_count = 0;
	ascii_num = 0;
	ascii_neg = 0;
	ascii_count = 0;
	return 0;
}


static void newdata(const unsigned char *data, int len)
{
	packet_parse(data, len);
	ascii_parse(data, len);
	// TODO: learn which one and skip the other
}



#if defined(LINUX) || defined(MACOSX)

static int portfd=-1;

int port_is_open(void)
{
	if (portfd > 0) return 1;
	return 0;
}

int open_port(const char *name)
{
	struct termios termsettings;
	int r;

	portfd = open(name, O_RDWR | O_NONBLOCK);
	if (portfd < 0) return 0;
	r = tcgetattr(portfd, &termsettings);
	if (r < 0) {
		close_port();
		return 0;
	}
	cfmakeraw(&termsettings);
	cfsetspeed(&termsettings, B115200);
	r = tcsetattr(portfd, TCSANOW, &termsettings);
	if (r < 0) {
		close_port();
		return 0;
	}
	return 1;
}

int read_serial_data(void)
{
	unsigned char buf[256];
	static int nodata_count=0;
	int n;

	if (portfd < 0) return -1;
	while (1) {
		n = read(portfd, buf, sizeof(buf));
		if (n > 0 && n <= int(sizeof(buf))) {
			newdata(buf, n);
			nodata_count = 0;
			return n;
		} else if (n == 0) {
			if (++nodata_count > 6) {
				close_port();
				nodata_count = 0;
				close_port();
				return -1;
			}
			return 0;
		} else {
			n = errno;
			if (n == EAGAIN) {
				return 0;
			} else if (n == EINTR) {
			} else {
				close_port();
				return -1;
			}
		}
	}
}

int write_serial_data(const void *ptr, int len)
{
	int n, written=0;
	fd_set wfds;
	struct timeval tv;

	//printf("Write %d\n", len);
	if (portfd < 0) return -1;
	while (written < len) {
		n = write(portfd, (const char *)ptr + written, len - written);
		if (n < 0 && (errno == EAGAIN || errno == EINTR)) n = 0;
		//printf("Write, n = %d\n", n);
		if (n < 0) return -1;
		if (n > 0) {
			written += n;
		} else {
			tv.tv_sec = 0;
			tv.tv_usec = 5000;
			FD_ZERO(&wfds);
			FD_SET(portfd, &wfds);
			n = select(portfd+1, NULL, &wfds, NULL, &tv);
			if (n < 0 && errno == EINTR) n = 1;
			if (n <= 0) return -1;
		}
	}
	return written;
}

void close_port(void)
{
	if (portfd >= 0) {
		close(portfd);
		portfd = -1;
	}
}

#elif defined(WINDOWS)

static HANDLE port_handle=INVALID_HANDLE_VALUE;

int port_is_open(void)
{
	if (port_handle == INVALID_HANDLE_VALUE) return 0;
	return 1;
}

int open_port(const char *name)
{
	COMMCONFIG port_cfg;
	COMMTIMEOUTS timeouts;
	DWORD len;
	char buf[64];
	int n;

	if (strncmp(name, "COM", 3) == 0 && sscanf(name + 3, "%d", &n) == 1) {
		snprintf(buf, sizeof(buf), "\\\\.\\COM%d", n);
		name = buf;
	}
	port_handle = CreateFileA(name, GENERIC_READ | GENERIC_WRITE,
		0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (port_handle == INVALID_HANDLE_VALUE) {
		return 0;
	}
	len = sizeof(COMMCONFIG);
	if (!GetCommConfig(port_handle, &port_cfg, &len)) {
		CloseHandle(port_handle);
		port_handle = INVALID_HANDLE_VALUE;
		return 0;
	}
	port_cfg.dcb.BaudRate = 115200;
	port_cfg.dcb.fBinary = TRUE;
	port_cfg.dcb.fParity = FALSE;
	port_cfg.dcb.fOutxCtsFlow = FALSE;
	port_cfg.dcb.fOutxDsrFlow = FALSE;
	port_cfg.dcb.fDtrControl = DTR_CONTROL_DISABLE;
	port_cfg.dcb.fDsrSensitivity = FALSE;
	port_cfg.dcb.fTXContinueOnXoff = TRUE;  // ???
	port_cfg.dcb.fOutX = FALSE;
	port_cfg.dcb.fInX = FALSE;
	port_cfg.dcb.fErrorChar = FALSE;
	port_cfg.dcb.fNull = FALSE;
	port_cfg.dcb.fRtsControl = RTS_CONTROL_DISABLE;
	port_cfg.dcb.fAbortOnError = FALSE;
	port_cfg.dcb.ByteSize = 8;
	port_cfg.dcb.Parity = NOPARITY;
	port_cfg.dcb.StopBits = ONESTOPBIT;
	if (!SetCommConfig(port_handle, &port_cfg, sizeof(COMMCONFIG))) {
		CloseHandle(port_handle);
		port_handle = INVALID_HANDLE_VALUE;
		return 0;
	}
	if (!EscapeCommFunction(port_handle, CLRDTR | CLRRTS)) {
		CloseHandle(port_handle);
		port_handle = INVALID_HANDLE_VALUE;
		return 0;
	}
        timeouts.ReadIntervalTimeout            = MAXDWORD;
        timeouts.ReadTotalTimeoutMultiplier     = 0;
        timeouts.ReadTotalTimeoutConstant       = 0;
        timeouts.WriteTotalTimeoutMultiplier    = 0;
        timeouts.WriteTotalTimeoutConstant      = 0;
        if (!SetCommTimeouts(port_handle, &timeouts)) {
		CloseHandle(port_handle);
		port_handle = INVALID_HANDLE_VALUE;
		return 0;
	}
	if (!EscapeCommFunction(port_handle, SETDTR)) {
		CloseHandle(port_handle);
		port_handle = INVALID_HANDLE_VALUE;
		return 0;
	}
	return 1;
}

int read_serial_data(void)
{
	COMSTAT st;
	DWORD errmask=0, num_read, num_request;
	OVERLAPPED ov;
	unsigned char buf[256];
	int r;

	if (port_handle == INVALID_HANDLE_VALUE) return -1;
	while (1) {
		if (!ClearCommError(port_handle, &errmask, &st)) {
			r = -1;
			break;
		}
		//printf("Read, %d requested, %lu buffered\n", count, st.cbInQue);
		if (st.cbInQue <= 0) {
			r = 0;
			break;
		}
		// now do a ReadFile, now that we know how much we can read
		// a blocking (non-overlapped) read would be simple, but win32
		// is all-or-nothing on async I/O and we must have it enabled
		// because it's the only way to get a timeout for WaitCommEvent
		if (st.cbInQue < (DWORD)sizeof(buf)) {
			num_request = st.cbInQue;
		} else {
			num_request = (DWORD)sizeof(buf);
		}
		ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (ov.hEvent == NULL) {
			close_port();
			return -1;
		}
		ov.Internal = ov.InternalHigh = 0;
		ov.Offset = ov.OffsetHigh = 0;
		if (ReadFile(port_handle, buf, num_request, &num_read, &ov)) {
			// this should usually be the result, since we asked for
			// data we knew was already buffered
			//printf("Read, immediate complete, num_read=%lu\n", num_read);
			r = num_read;
		} else {
			if (GetLastError() == ERROR_IO_PENDING) {
				if (GetOverlappedResult(port_handle, &ov, &num_read, TRUE)) {
					//printf("Read, delayed, num_read=%lu\n", num_read);
					r = num_read;
				} else {
					//printf("Read, delayed error\n");
					r = -1;
				}
			} else {
				//printf("Read, error\n");
				r = -1;
			}
		}
		CloseHandle(ov.hEvent);
		if (r <= 0) break;
		newdata(buf, r);
	}
	if (r < 0) {
		CloseHandle(port_handle);
		port_handle = INVALID_HANDLE_VALUE;
	}
        return r;
}

int write_serial_data(const void *ptr, int len)
{
	DWORD num_written;
	OVERLAPPED ov;
	int r;

	ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (ov.hEvent == NULL) return -1;
	ov.Internal = ov.InternalHigh = 0;
	ov.Offset = ov.OffsetHigh = 0;
	if (WriteFile(port_handle, ptr, len, &num_written, &ov)) {
		//printf("Write, immediate complete, num_written=%lu\n", num_written);
		r = num_written;
	} else {
		if (GetLastError() == ERROR_IO_PENDING) {
			if (GetOverlappedResult(port_handle, &ov, &num_written, TRUE)) {
			//printf("Write, delayed, num_written=%lu\n", num_written);
			r = num_written;
			} else {
				//printf("Write, delayed error\n");
				r = -1;
			}
		} else {
			//printf("Write, error\n");
			r = -1;
		}
	};
	CloseHandle(ov.hEvent);
	return r;
}

void close_port(void)
{
	CloseHandle(port_handle);
	port_handle = INVALID_HANDLE_VALUE;
}


#endif

static float cal_data_sent[19];
static int cal_confirm_needed = 0;

static int is_float_ok(float actual, float expected)
{
	float err, maxerr;

	err = fabsf(actual - expected);
	maxerr = 0.0001f + fabsf(expected) * 0.00003f;
	if (err <= maxerr) return 1;
	return 0;
}

void cal1_data(const float* data)
{
	int i, ok;

	if (cal_confirm_needed) {
#if 0
		printf("expected cal1: ");
		for (i = 0; i < 10; i++) {
			printf("  %.5f,", cal_data_sent[i]);
		}
		printf("\ngot cal1_data: ");
		for (i = 0; i < 10; i++) {
			printf("  %.5f,", data[i]);
		}
		printf("\n");
#endif
		ok = 1;
		for (i = 0; i < 10; i++) {
			if (!is_float_ok(data[i], cal_data_sent[i])) ok = 0;
		}
		if (ok) {
			cal_confirm_needed &= ~1; // got cal1 confirm
			if (cal_confirm_needed == 0) {
				calibration_confirmed();
			}
		}
	}
}

void cal2_data(const float* data)
{
	int i, ok;

	if (cal_confirm_needed) {
#if 0
		printf("expected cal2: ");
		for (i = 0; i < 9; i++) {
			printf(" %.5f,", cal_data_sent[i + 10]);
		}
		printf("\ngot cal2_data: ");
		for (i = 0; i < 9; i++) {
			printf(" %.5f,", data[i]);
		}
		printf("\n");
#endif
		ok = 1;
		for (i = 0; i < 9; i++) {
			if (!is_float_ok(data[i], cal_data_sent[i + 10])) ok = 0;
		}
		if (ok) {
			cal_confirm_needed &= ~2; // got cal2 confirm
			if (cal_confirm_needed == 0) {
				calibration_confirmed();
			}
		}
	}
}

static uint16_t crc16(uint16_t crc, uint8_t data)
{
	unsigned int i;

	crc ^= data;
	for (i = 0; i < 8; ++i) {
		if (crc & 1) {
			crc = (crc >> 1) ^ 0xA001;
		}
		else {
			crc = (crc >> 1);
		}
	}
	return crc;
}

static uint8_t* copy_lsb_first(uint8_t* dst, float f)
{
	union {
		float f;
		uint32_t n;
	} data;

	data.f = f;
	*dst++ = data.n;
	*dst++ = data.n >> 8;
	*dst++ = data.n >> 16;
	*dst++ = data.n >> 24;
	return dst;
}

int send_calibration(void)
{
	uint8_t* p, buf[68];
	uint16_t crc;
	int i;

	p = buf;
	*p++ = 117; // 2 byte signature
	*p++ = 84;
	for (i = 0; i < 3; i++) {
		p = copy_lsb_first(p, 0.0f); // accelerometer offsets
		cal_data_sent[0 + i] = 0.0f;
	}
	for (i = 0; i < 3; i++) {
		p = copy_lsb_first(p, 0.0f); // gyroscope offsets
		cal_data_sent[3 + i] = 0.0f;
	}
	for (i = 0; i < 3; i++) {
		p = copy_lsb_first(p, magcal.V[i]); // 12 bytes offset/hardiron
		cal_data_sent[6 + i] = magcal.V[i];
	}
	p = copy_lsb_first(p, magcal.B); // field strength
	p = copy_lsb_first(p, magcal.invW[0][0]); //10
	p = copy_lsb_first(p, magcal.invW[1][1]); //11
	p = copy_lsb_first(p, magcal.invW[2][2]); //12
	p = copy_lsb_first(p, magcal.invW[0][1]); //13
	p = copy_lsb_first(p, magcal.invW[0][2]); //14
	p = copy_lsb_first(p, magcal.invW[1][2]); //15
	cal_data_sent[9] = magcal.B;
	cal_data_sent[10] = magcal.invW[0][0];
	cal_data_sent[11] = magcal.invW[0][1];
	cal_data_sent[12] = magcal.invW[0][2];
	cal_data_sent[13] = magcal.invW[1][0];
	cal_data_sent[14] = magcal.invW[1][1];
	cal_data_sent[15] = magcal.invW[1][2];
	cal_data_sent[16] = magcal.invW[2][0];
	cal_data_sent[17] = magcal.invW[2][1];
	cal_data_sent[18] = magcal.invW[2][2];
	cal_confirm_needed = 3;
	crc = 0xFFFF;
	for (i = 0; i < 66; i++) {
		crc = crc16(crc, buf[i]);
	}
	*p++ = crc;   // 2 byte crc check
	*p++ = crc >> 8;
	return write_serial_data(buf, 68);
}