#include "fl_app_com.h"
#include "src/fl_app_lights.h"

#define CONFIG_NUM_DIGITS 6

fl_app_com::fl_app_com(const QString portname, bool is_legacy_protocol) :
    is_legacy_protocol(is_legacy_protocol)
{
    if(!is_legacy_protocol) {
        buf_tx = new uint8_t[BUF_TX_SIZE];
        _transfer = &fl_app_com::regular_transfer;
    } else {
        buf_tx = new uint8_t[BUF_TX_SIZE_LEGACY];
        _transfer = &fl_app_com::legacy_transfer;
    }
    buf_rx = new uint8_t[BUF_RX_SIZE];

    QIODevice::OpenMode open_mode;
    serial_port.close();
    serial_port.setPortName(portname);
    serial_port.setBaudRate(QSerialPort::BaudRate::Baud115200);
    serial_port.setDataBits(QSerialPort::DataBits::Data8);
    serial_port.setParity(QSerialPort::Parity::NoParity);
    serial_port.setStopBits(QSerialPort::StopBits::OneStop);

    serial_port.open(open_mode);
}

fl_app_com::~fl_app_com() {
    serial_port.close();
    delete buf_rx;
    delete buf_tx;
}

void fl_app_com::transfer_serial0(uint8_t *clr_arr)
{
    transfer_serial(clr_arr, 0);
}

void fl_app_com::transfer_serial1(uint8_t *clr_arr)
{
    transfer_serial(clr_arr, 1);
}

void fl_app_com::transfer_light_pattern(uint8_t instance, uint8_t param0, uint8_t param1)
{
    clear_buffer();
    if(!is_legacy_protocol) {
        set_control_byte(0x04);
        buf_tx[FL_APP_COM_DATA_OFFSET] = instance;
        buf_tx[FL_APP_COM_DATA_OFFSET + 1] = param0;
        buf_tx[FL_APP_COM_DATA_OFFSET + 2] = param1;
    } else {
        set_control_byte(0x20);
        buf_tx[FL_APP_COM_DATA_OFFSET] = instance;
        buf_tx[FL_APP_COM_DATA_OFFSET + 1] = param0;
    }
    transfer();
}

void fl_app_com::transfer_gui_set(uint8_t instance, uint8_t param0)
{
    clear_buffer();
    set_control_byte(0x10);
    buf_tx[FL_APP_COM_DATA_OFFSET] = instance;
    buf_tx[FL_APP_COM_DATA_OFFSET + 1] = param0;
    transfer();
}

void fl_app_com::legacy_transfer_clock_flags(uint8_t clock_flag, uint8_t clock_flag_value)
{
    clear_buffer();
    set_control_byte(0x12);
    buf_tx[FL_APP_COM_DATA_OFFSET] = clock_flag;
    buf_tx[FL_APP_COM_DATA_OFFSET + 1] = clock_flag_value;
    transfer();
}

void fl_app_com::transfer_time_date(vfdco_time_t t, vfdco_date_t d)
{
    clear_buffer();
    set_control_byte(is_legacy_protocol ? 0x10 : 0x20);
    buf_tx[FL_APP_COM_DATA_OFFSET]     = t.s;
    buf_tx[FL_APP_COM_DATA_OFFSET + 1] = t.m;
    buf_tx[FL_APP_COM_DATA_OFFSET + 2] = t.h;
    buf_tx[FL_APP_COM_DATA_OFFSET + 3] = d.d;
    buf_tx[FL_APP_COM_DATA_OFFSET + 4] = d.m;
    buf_tx[FL_APP_COM_DATA_OFFSET + 5] = d.y;
    buf_tx[FL_APP_COM_DATA_OFFSET + 6] = 0x23;
    transfer();
}

void fl_app_com::transfer_brightness(uint8_t disp_or_led, uint8_t dim_factor)
{
    clear_buffer();
    set_control_byte(0x21);
    buf_tx[FL_APP_COM_DATA_OFFSET]     = disp_or_led;
    buf_tx[FL_APP_COM_DATA_OFFSET + 1] = dim_factor;
    transfer();
}

void fl_app_com::transfer_night_shift(vfdco_time_t start, vfdco_time_t stop)
{
    clear_buffer();
    set_control_byte(0x22);
    buf_tx[FL_APP_COM_DATA_OFFSET]     = start.h;
    buf_tx[FL_APP_COM_DATA_OFFSET + 1] = start.m;
    buf_tx[FL_APP_COM_DATA_OFFSET + 2] = stop.h;
    buf_tx[FL_APP_COM_DATA_OFFSET + 3] = stop.m;
    transfer();
}

void fl_app_com::legacy_transfer_night_shift(vfdco_time_t start, vfdco_time_t stop, uint8_t en1, uint8_t en2)
{
    clear_buffer();
    set_control_byte(0x11);
    buf_tx[FL_APP_COM_DATA_OFFSET] = 0x23;
    buf_tx[FL_APP_COM_DATA_OFFSET + 1] = start.m;
    buf_tx[FL_APP_COM_DATA_OFFSET + 2] = start.h;
    buf_tx[FL_APP_COM_DATA_OFFSET + 3] = stop.m;
    buf_tx[FL_APP_COM_DATA_OFFSET + 3] = stop.h;
    buf_tx[FL_APP_COM_DATA_OFFSET + 5] = en1;
    buf_tx[FL_APP_COM_DATA_OFFSET + 6] = en2;
    buf_tx[FL_APP_COM_DATA_OFFSET + 7] = 0x23;
    transfer();
}

void fl_app_com::transfer_welcome_set(uint8_t *message)
{
    clear_buffer();
    set_control_byte(is_legacy_protocol ? 0x21 : 0x25);
    memcpy(buf_tx + FL_APP_COM_DATA_OFFSET, message, CONFIG_NUM_DIGITS);
    transfer();
}

void fl_app_com::transfer_message(uint8_t *m1, uint8_t *m2, uint8_t *m3, uint8_t *m4, uint8_t duration)
{
    clear_buffer();
    if(!is_legacy_protocol) {
        set_control_byte(0x26);
        if(m4) memcpy(buf_tx + FL_APP_COM_DATA_OFFSET + CONFIG_NUM_DIGITS * 3, m4, CONFIG_NUM_DIGITS);
    } else {
        set_control_byte(0x1F);
        buf_tx[20] = duration;
        buf_tx[21] = m2 ? 1 : 0;
        buf_tx[22] = m3 ? 1 : 0;
    }
    memcpy(buf_tx + FL_APP_COM_DATA_OFFSET, m1, CONFIG_NUM_DIGITS);
    if(m2) memcpy(buf_tx + FL_APP_COM_DATA_OFFSET + CONFIG_NUM_DIGITS, m2, CONFIG_NUM_DIGITS);
    if(m3) memcpy(buf_tx + FL_APP_COM_DATA_OFFSET + CONFIG_NUM_DIGITS * 2, m3, CONFIG_NUM_DIGITS);
    transfer();
}

void fl_app_com::transfer_clock_control(fl_app_com_control_t control)
{
    clear_buffer();
    if(!is_legacy_protocol) {
        set_control_byte(0x30);
        buf_tx[FL_APP_COM_DATA_OFFSET] = (uint8_t)control;
    } else {
        switch(control) {
            case FL_APP_COM_CONTROL_FW_VERSION_REQ: set_control_byte(0x22); break;
            case FL_APP_COM_CONTROL_SETTINGS_SAVE_REQ: set_control_byte(0x33); break;
            case FL_APP_COM_CONTROL_DEFAULT_REQ: set_control_byte(0x34); break;
            default: return;
        }
    }
    transfer();
    // Wait to receive
}

void fl_app_com::clear_buffer()
{
    if(!is_legacy_protocol) memset(buf_tx, 0x00, BUF_TX_SIZE);
    else memset(buf_tx, 0x00, BUF_TX_SIZE_LEGACY);
    memset(buf_rx, 0x00, BUF_RX_SIZE);
}

void fl_app_com::set_control_byte(uint8_t value)
{
    buf_tx[FL_APP_COM_CONTROL_BYTE] = value;
}

void fl_app_com::transfer_serial(uint8_t *clr_arr, uint8_t mode)
{
    clear_buffer();
    if(!is_legacy_protocol) {
        set_control_byte(mode ? 0x01 : 0x00);
        memcpy(buf_tx + FL_APP_COM_DATA_OFFSET, clr_arr, CONFIG_NUM_BYTES);
        transfer();
    } else {
        set_control_byte(mode ? 0x02 : 0x01);
        uint8_t *buf_tx_alt = buf_tx + FL_APP_COM_DATA_OFFSET;
        for(uint_fast8_t i = 0; i < CONFIG_NUM_BYTES; ++i) {
            buf_tx_alt[3 * i] = clr_arr[4 * i + 1]; // G
            buf_tx_alt[3 * i + 1] = clr_arr[4 * i]; // R
            buf_tx_alt[3 * i + 2] = clr_arr[4 * i + 2]; // B
        }
        legacy_transfer();
    }
}

void fl_app_com::transfer()
{
    (this->*_transfer)();
}

void fl_app_com::regular_transfer()
{
    buf_tx[0] = 0x24;
    buf_tx[BUF_TX_SIZE - 1] = 0x25;
    // Replace this
    qDebug("Transfer");
    for(uint8_t i = 0; i < BUF_TX_SIZE; ++i) {
        qDebug("%hhu: 0x%hhx, %hhu", i, buf_tx[i], buf_tx[i]);
    }
}

void fl_app_com::legacy_transfer()
{
    buf_tx[0] = 0x23;
    buf_tx[BUF_TX_SIZE - 1] = 0x24;
    // Replace this
    qDebug("Legacy Transfer");
    for(uint8_t i = 0; i < BUF_TX_SIZE_LEGACY; ++i) {
        qDebug("%hhu: 0x%hhx, %hhu", i, buf_tx[i], buf_tx[i]);
    }
}