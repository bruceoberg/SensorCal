#include "serialdata.h"
#include "libcalib/mag_calibrator.h"
#include <stdint.h>
#include <math.h>
#include "portscanner.h"

// --- Calibration send/confirm logic ---

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

int send_calibration()
{
	const auto & calib = libcalib::Mag::CCalibrator::Ensure();
	const auto & fitter = calib.m_fitter;

	const auto & vecV = fitter.m_cal.m_vecV;
	const auto & matWInv = fitter.m_cal.m_matWInv;
	const auto & sB = fitter.m_cal.m_sB;

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
		p = copy_lsb_first(p, vecV[i]); // 12 bytes offset/hardiron
		cal_data_sent[6 + i] = vecV[i];
	}
	p = copy_lsb_first(p, sB); // field strength
	p = copy_lsb_first(p, matWInv[0][0]); //10
	p = copy_lsb_first(p, matWInv[1][1]); //11
	p = copy_lsb_first(p, matWInv[2][2]); //12
	p = copy_lsb_first(p, matWInv[0][1]); //13
	p = copy_lsb_first(p, matWInv[0][2]); //14
	p = copy_lsb_first(p, matWInv[1][2]); //15
	cal_data_sent[9] = sB;
	cal_data_sent[10] = matWInv[0][0];
	cal_data_sent[11] = matWInv[0][1];
	cal_data_sent[12] = matWInv[0][2];
	cal_data_sent[13] = matWInv[1][0];
	cal_data_sent[14] = matWInv[1][1];
	cal_data_sent[15] = matWInv[1][2];
	cal_data_sent[16] = matWInv[2][0];
	cal_data_sent[17] = matWInv[2][1];
	cal_data_sent[18] = matWInv[2][2];
	cal_confirm_needed = 3;
	crc = 0xFFFF;
	for (i = 0; i < 66; i++) {
		crc = crc16(crc, buf[i]);
	}
	*p++ = crc;   // 2 byte crc check
	*p++ = crc >> 8;
	return static_cast<int>(Scanner().WriteToActive(68, buf));
}
