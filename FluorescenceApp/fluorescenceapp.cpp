#include "fluorescenceapp.h"
#include "ui_fluorescenceapp.h"

#include "src/fl_app_time.h"
#include "src/fl_app_lights.h"

#include <QtSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QDesktopServices>
#include <QMessageBox>

#include "QtColorWidgets/colorwidgets_global.hpp"

using namespace color_widgets;

// Global dynamic colors
QWidget *preset_dynamic_colors[NUM_PRESET_DYNAMIC_COLORS];
QWidget *preset_dynamic_time[NUM_PRESET_DYNAMIC_TIME];
uint_fast8_t preset_dynamic_timer = 0;

Light_Pattern global_lights_instance;

// Global time updater, ll support
QTimer *global_timer = nullptr;
vfdco_time_t global_time;

// Todo replace bool by fancy function
bool global_is_fw2 = false;

QString bliss_descriptions[6] = {
    "Nordlicht means northern light in German. Temperament: mysterious, majestic, spirited, impressive\n\
Ink: fluorescent green to teal, touches of purple, Song: Higher Ground by ODESZA",

    "Pastellfrühling means pastel spring in German. Temperament: delightful, lovely, awakening, inspirational\n\
Ink: pastel. cherry, pink, some light green and rarely drip of light blue. Taste: Cherry and honey",

    "Hummelhonig means bumble bee honey in German. Temperament: lively, energetic, active, peaceful\n\
Ink: highly saturated green and blue gradients. Song: Here We Are by BRVTHR",

    "Meeresgeflüster means whispers of the sea in German. Temperament: light-hearted, carefree, liberating, calm\n\
Ink: watercolor. light sky blue to turquoise, with warm white sparks. Song: Island by Unknown Neighbour",

    "Herbstlagerfeuer means fall camp fire in German. Temperament: cozy, sentimental, sincere, warm\n\
Ink: acrylic. lots of orange and strong yellow tones. rarely some green and brick red. Song: Portland\n\
by Andrea von Kampen. Taste: pumpkin, blood orange, maple, cinnamon",

    "Abendhimmel means evening sky in German. Temperament: passionate, untamed, infinite, intense\n\
Ink: strong red. every warm red tone, some orange, some magenta. Song: Lost In The Night - THBD"
};

FluorescenceApp::FluorescenceApp(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::FluorescenceApp)
{
    ui->setupUi(this);
    this->setWindowTitle("Fluorescence App by The VFD Collective");
    this->setFixedSize(this->size());

    ui->com_connect->setText("►");
    // List all available serial ports to com_select + empty
    ui->com_select->addItem("");
    for(QSerialPortInfo available_ports : QSerialPortInfo::availablePorts()) {
        ui->com_select->addItem(available_ports.portName());
    }
    hide_all_panels();
    ui->panel_welcome->show();

    // Init timer
    global_timer = new QTimer(this);
    connect(global_timer, &QTimer::timeout, this, QOverload<>::of(&FluorescenceApp::update));
    global_timer->start(50);

    // Init custom color wheel & white
    ui->custom_color_wheel->setDisplayFlag(ColorWheel::COLOR_FLAGS, ColorWheel::COLOR_HSL);
    this->custom_global_color.setHsl(0, 255, 127);
    this->custom_global_color_white = 0;

    ui->custom_slider_w->setValue(custom_global_color_white);
    ui->custom_slider_w->setFirstColor(QColor(0, 0, 0, 0));
    ui->custom_slider_w->setLastColor(QColor(254, 204, 102, 255));

    custom_color_update_all_sliders(0);

    // Init dynamic colors & time. Dude this is so stupid but y know it does the job, kinda.
    QWidget *tmp_dynamic_colors[NUM_PRESET_DYNAMIC_COLORS] = {ui->dynamic_c1, ui->dynamic_c2, ui->dynamic_c3, ui->dynamic_c4, ui->dynamic_c5, ui->dynamic_c6};
    QWidget *tmp_dynamic_time[NUM_PRESET_DYNAMIC_TIME] = {
        ui->dynamic_t_1, ui->dynamic_t_2, ui->dynamic_t_3, ui->dynamic_t_4, ui->dynamic_t_5, ui->dynamic_t_6, ui->dynamic_t_7,
        ui->dynamic_t_8, ui->dynamic_t_9, ui->dynamic_t_10, ui->dynamic_t_11, ui->dynamic_t_12, ui->dynamic_t_13, ui->dynamic_t_14,
        ui->dynamic_t_15, ui->dynamic_t_16, ui->dynamic_t_17, ui->dynamic_t_18, ui->dynamic_t_19, ui->dynamic_t_20, ui->dynamic_t_21
    };
    memcpy(preset_dynamic_colors, tmp_dynamic_colors, NUM_PRESET_DYNAMIC_COLORS * sizeof(QWidget *));
    memcpy(preset_dynamic_time, tmp_dynamic_time, NUM_PRESET_DYNAMIC_TIME * sizeof(QWidget *));
    hide_all_dynamic_control_panels();
    clear_lights_instance();
    Light_Pattern_Spectrum_Init((struct Light_Pattern_Spectrum *)&global_lights_instance, NULL);
    ui->panel_dyn_spectrum->show();
    // Set description of Bliss Moment by default
    on_panel_dyn_bliss_param_currentIndexChanged(0);
}

FluorescenceApp::~FluorescenceApp()
{
    delete global_timer;
    delete ui;
}

void FluorescenceApp::update(){
    // Display current time!
    QTime ct = QTime::currentTime();
    QDate cd = QDate::currentDate();
    ui->timesync_time_label->setText(ct.toString("hh:mm:ss"));
    ui->timesync_date_label->setText(cd.toString(Qt::DefaultLocaleLongDate));

    // Update color wheel
    vfdco_date_t d;
    vfdco_get_date_time(&d, &global_time);
    Light_Pattern_Update(&global_lights_instance);
    ++preset_dynamic_timer;
    for(uint_fast8_t i = 0; i < NUM_PRESET_DYNAMIC_TIME; ++i) {
        uint_fast8_t hpdt = preset_dynamic_timer / 8;
        if(i < hpdt) dynamic_cast<ColorPreview *>(preset_dynamic_time[i])->setColor(QColor::fromRgb(16, 128, 128));
        else dynamic_cast<ColorPreview *>(preset_dynamic_time[i])->setColor(QColor::fromRgb(196, 196, 196));
    }
    if(preset_dynamic_timer == (NUM_PRESET_DYNAMIC_TIME * 8)) {
        preset_dynamic_timer = 0;
    }

}

void FluorescenceApp::hide_all_panels() {
    ui->panel_welcome->hide();
    ui->panel_custom_colors->hide();
    ui->panel_presets->hide();
    ui->panel_message->hide();
    ui->panel_timesync->hide();
    ui->panel_settings->hide();
    ui->panel_lsettings->hide();
}

void FluorescenceApp::on_com_connect_clicked()
{
    if(ui->com_label_connect->text() == "Connect") {
        global_com_instance = new fl_app_com(ui->com_label_connect->text(), global_is_fw2);

        ui->panel_main_control->setEnabled(true);
        ui->com_label_connect->setText("Disconnect");
        ui->com_connect->setText("◼");
    } else if(ui->com_label_connect->text() == "Disconnect") {
        delete global_com_instance;
        global_com_instance = nullptr;

        ui->com_select->setCurrentIndex(0); // Empty
        ui->com_label_connect->setText("Connect");
        ui->com_connect->setText("►");
        ui->panel_main_control->setEnabled(false);

        hide_all_panels();
        ui->panel_welcome->show();
    } else {
        // Oops, you should never get to this point
    }
}

void FluorescenceApp::on_main_preset_clicked()
{
    hide_all_panels();
    ui->panel_presets->show();
}

void FluorescenceApp::on_main_custom_clicked()
{
    hide_all_panels();
    ui->panel_custom_colors->show();
}

void FluorescenceApp::on_main_timesync_clicked()
{
    hide_all_panels();
    ui->panel_timesync->show();
}

void FluorescenceApp::on_main_message_clicked()
{
    hide_all_panels();
    ui->panel_message->show();
}

void FluorescenceApp::on_main_settings_clicked()
{
    hide_all_panels();
    if(!global_is_fw2) {
        ui->panel_settings->show();
    } else { // Legacy (fw2) Settings
        ui->panel_lsettings->show();
    }

}


void FluorescenceApp::custom_color_update_all_sliders(bool block_color_wheel) {
    // Mutual exclusion
    bool blocked = signalsBlocked();
    blockSignals(true);
    Q_FOREACH(QWidget* w, findChildren<QWidget*>())
        w->blockSignals(true);

    // Sync elements
    if(!block_color_wheel) ui->custom_color_wheel->setColor(custom_global_color);

    ui->custom_value_h->setValue((double)custom_global_color.hueF() * 359.0f);
    ui->custom_value_s->setValue(custom_global_color.hslSaturationF() * 255.0f);
    ui->custom_value_l->setValue(custom_global_color.lightnessF() * 255.0f);
    ui->custom_value_r->setValue(custom_global_color.red());
    ui->custom_value_g->setValue(custom_global_color.green());
    ui->custom_value_b->setValue(custom_global_color.blue());

    ui->custom_slider_h->setColorHue(custom_global_color.hslHueF());
    ui->custom_slider_s->setValue(custom_global_color.hslSaturationF() * 255.0f);
    ui->custom_slider_s->setFirstColor(QColor::fromHslF(custom_global_color.hueF(), 0, custom_global_color.lightnessF()));
    ui->custom_slider_s->setLastColor(QColor::fromHslF(custom_global_color.hueF(), 1, custom_global_color.lightnessF()));
    ui->custom_slider_l->setValue(custom_global_color.lightnessF() * 255.0f);
    ui->custom_slider_l->setFirstColor(QColor::fromHsvF(custom_global_color.hueF(), custom_global_color.saturationF(), 0));
    ui->custom_slider_l->setLastColor(QColor::fromHsvF(custom_global_color.hueF(), custom_global_color.saturationF(), 1));

    ui->custom_slider_r->setValue(custom_global_color.red());
    ui->custom_slider_r->setFirstColor(QColor(0, custom_global_color.green(), custom_global_color.blue()));
    ui->custom_slider_r->setLastColor(QColor(255, custom_global_color.green(), custom_global_color.blue()));
    ui->custom_slider_g->setValue(custom_global_color.green());
    ui->custom_slider_g->setFirstColor(QColor(custom_global_color.red(), 0, custom_global_color.blue()));
    ui->custom_slider_g->setLastColor(QColor(custom_global_color.red(), 255, custom_global_color.blue()));
    ui->custom_slider_b->setValue(custom_global_color.blue());
    ui->custom_slider_b->setFirstColor(QColor(custom_global_color.red(), custom_global_color.green(), 0));
    ui->custom_slider_b->setLastColor(QColor(custom_global_color.red(), custom_global_color.green(), 255));

    ui->custom_value_hex->setColor(custom_global_color);

    ui->custom_value_w->setValue(custom_global_color_white);
    ui->custom_slider_w->setValue(custom_global_color_white);

    // Begin Transfer
    if(global_com_instance) {
        QCheckBox *ch[6] = {
            ui->custom_select_led_1, ui->custom_select_led_2, ui->custom_select_led_3,
            ui->custom_select_led_4, ui->custom_select_led_5, ui->custom_select_led_6
        };
        uint8_t color_arr[CONFIG_NUM_BYTES] = {0};
        for(uint_fast8_t i = 0; i < CONFIG_NUM_PIXELS; ++i) {
            if(ch[i]->checkState() == Qt::Checked) {
                custom_led_colors[i] = custom_global_color;
                custom_led_white[i] = custom_global_color_white;
            }
            color_arr[4 * i] = custom_led_colors[i].red();
            color_arr[4 * i + 1] = custom_led_colors[i].green();
            color_arr[4 * i + 2] = custom_led_colors[i].blue();
            color_arr[4 * i + 3] = custom_led_white[i];
        }
        global_com_instance->transfer_serial0(color_arr);
    }

    blockSignals(blocked);
    Q_FOREACH(QWidget* w, findChildren<QWidget*>())
        w->blockSignals(false);
}

void FluorescenceApp::on_custom_slider_r_sliderMoved(int position)
{
    if (!signalsBlocked()) {
        custom_global_color.setRed(position);
        custom_color_update_all_sliders(0);
    }
}
void FluorescenceApp::on_custom_slider_g_sliderMoved(int position)
{
    if (!signalsBlocked()) {
        custom_global_color.setGreen(position);
        custom_color_update_all_sliders(0);
    }
}
void FluorescenceApp::on_custom_slider_b_sliderMoved(int position)
{
    if (!signalsBlocked()) {
        custom_global_color.setBlue(position);
        custom_color_update_all_sliders(0);
    }
}
void FluorescenceApp::on_custom_slider_w_sliderMoved(int position)
{
    if (!signalsBlocked()) {
        custom_global_color_white = position;
        custom_color_update_all_sliders(0);
    }
}
void FluorescenceApp::on_custom_slider_h_sliderMoved(int position)
{
    if (!signalsBlocked()) {
        QColor new_color;
        new_color.setHslF((double)position / 359.0f, custom_global_color.hslSaturationF(), custom_global_color.lightnessF());
        custom_global_color = new_color;
        custom_color_update_all_sliders(0);
    }
}
void FluorescenceApp::on_custom_slider_s_sliderMoved(int position)
{
    if (!signalsBlocked()) {
        QColor new_color;
        new_color.setHslF(custom_global_color.hueF(), position / 255.0f, custom_global_color.lightnessF());
        custom_global_color = new_color;
        custom_color_update_all_sliders(0);
    }
}
void FluorescenceApp::on_custom_slider_l_sliderMoved(int position)
{
    if (!signalsBlocked()) {
        QColor new_color;
        new_color.setHslF(custom_global_color.hueF(), custom_global_color.hslSaturationF(), position / 255.0f);
        custom_global_color = new_color;
        custom_color_update_all_sliders(0);
    }
}
void FluorescenceApp::on_custom_value_hex_colorEditingFinished(const QColor &color)
{
    if (!signalsBlocked()) {
        custom_global_color = color;
        custom_color_update_all_sliders(0);
    }
}


void FluorescenceApp::on_custom_value_r_valueChanged(int arg1) { on_custom_slider_r_sliderMoved(arg1); }
void FluorescenceApp::on_custom_value_g_valueChanged(int arg1) { on_custom_slider_g_sliderMoved(arg1); }
void FluorescenceApp::on_custom_value_b_valueChanged(int arg1) { on_custom_slider_b_sliderMoved(arg1); }
void FluorescenceApp::on_custom_value_w_valueChanged(int arg1) { on_custom_slider_w_sliderMoved(arg1); }
void FluorescenceApp::on_custom_value_h_valueChanged(int arg1) { on_custom_slider_h_sliderMoved(arg1); }
void FluorescenceApp::on_custom_value_s_valueChanged(int arg1) { on_custom_slider_s_sliderMoved(arg1); }
void FluorescenceApp::on_custom_value_l_valueChanged(int arg1) { on_custom_slider_l_sliderMoved(arg1); }

void FluorescenceApp::on_custom_color_wheel_colorSelected(const QColor &arg1)
{
    if (!signalsBlocked()) {
        custom_global_color = arg1;
        custom_color_update_all_sliders(1);
    }
}

void FluorescenceApp::clear_lights_instance() {
    Container_Light_Pattern_Clear(&global_lights_instance);
}

void FluorescenceApp::on_dynamic_spectrum_clicked() {
    clear_lights_instance();
    hide_all_dynamic_control_panels();
    Light_Pattern_Spectrum_Init((struct Light_Pattern_Spectrum *)&global_lights_instance, NULL);
    ui->panel_dyn_spectrum->show();
}

void FluorescenceApp::on_dynamic_bliss_dnc_clicked() {
    clear_lights_instance();
    hide_all_dynamic_control_panels();
    Light_Pattern_MomentsOfBliss_Init((struct Light_Pattern_MomentsOfBliss *)&global_lights_instance, NULL);
    if(!global_is_fw2) {
        ui->panel_dyn_bliss->show();
    } else { // Legacy (fw2)
        ui->panel_dyn_music->show();
    }
}

void FluorescenceApp::on_dynamic_rainbow_clicked() {
    clear_lights_instance();
    hide_all_dynamic_control_panels();
    Light_Pattern_Rainbow_Init((struct Light_Pattern_Rainbow *)&global_lights_instance, NULL);
    ui->panel_dyn_rnb->show();
}

void FluorescenceApp::on_dynamic_chase_clicked() {
    clear_lights_instance();
    hide_all_dynamic_control_panels();
    Light_Pattern_Chase_Init((struct Light_Pattern_Chase *)&global_lights_instance, &global_time, NULL);
    ui->panel_dyn_chase->show();
}

void FluorescenceApp::on_dynamic_timecode_clicked() {
    clear_lights_instance();
    hide_all_dynamic_control_panels();
    Light_Pattern_Time_Code_Init((struct Light_Pattern_Time_Code *)&global_lights_instance, &global_time);
    ui->panel_dyn_tcode->show();
}

void FluorescenceApp::on_dynamic_police_clicked() {
    clear_lights_instance();
    hide_all_dynamic_control_panels();
    Light_Pattern_Cop_Init((struct Light_Pattern_Cop *)&global_lights_instance);
    ui->panel_dyn_cop->show();
}

void FluorescenceApp::hide_all_dynamic_control_panels() {
    ui->panel_dyn_spectrum->hide();
    ui->panel_dyn_bliss->hide();
    ui->panel_dyn_music->hide();
    ui->panel_dyn_rnb->hide();
    ui->panel_dyn_chase->hide();
    ui->panel_dyn_tcode->hide();
    ui->panel_dyn_cop->hide();
}

void FluorescenceApp::on_panel_dyn_bliss_param_currentIndexChanged(int index)
{
    // Set moment description
    ui->panel_dyn_bliss_description->setText(bliss_descriptions[index]);
}

void FluorescenceApp::on_settings_td_set_clicked()
{
    // Direct parsing since indices correspond to mode enums. Two subsequent transfers
    uint8_t time_format = ui->settings_td_timemode->currentIndex();
    uint8_t date_format = ui->settings_td_datemode->currentIndex();
    global_com_instance->transfer_gui_set(0, time_format);
    global_com_instance->transfer_gui_set(1, date_format);
}

void FluorescenceApp::on_settings_bri_set_clicked()
{
    // Direct parsing, since indices correspond to brightness enums >> 1
    uint8_t brightness_display = ui->settings_bri_disp->currentIndex() << 1;
    uint8_t brightness_led = ui->settings_bri_led->currentIndex() << 1;
    global_com_instance->transfer_brightness(0, brightness_display);
    global_com_instance->transfer_brightness(1, brightness_led);
}

void FluorescenceApp::on_settings_nsh_enable_stateChanged(int arg1)
{
    if(arg1 == Qt::Checked) ui->settings_nsh_panel->setEnabled(true);
    else ui->settings_nsh_panel->setEnabled(true);
}

void FluorescenceApp::on_settings_nsh_set_clicked()
{
    vfdco_time_t t_start = {
        .h = (uint8_t)ui->settings_nsh_start->time().hour(),
        .m = (uint8_t)ui->settings_nsh_start->time().minute(),
        .s = 0
    };
    vfdco_time_t t_stop = {
        .h = (uint8_t)ui->settings_nsh_stop->time().hour(),
        .m = (uint8_t)ui->settings_nsh_stop->time().minute(),
        .s = 0
    };
    global_com_instance->transfer_night_shift(t_start, t_stop);
}

void FluorescenceApp::on_settings_settings_default_clicked() {
    global_com_instance->transfer_clock_control(FL_APP_COM_CONTROL_DEFAULT_REQ);
}

void FluorescenceApp::on_settings_settings_save_clicked()
{
    global_com_instance->transfer_clock_control(FL_APP_COM_CONTROL_SETTINGS_SAVE_REQ);
}

void FluorescenceApp::on_settings_info_download_clicked()
{
    // Just take you to the support
    QDesktopServices::openUrl(QUrl("https://www.thevfdcollective.com/support"));
}

void FluorescenceApp::on_lsettings_td_12hr_enable_stateChanged(int arg1)
{
    if(arg1 == Qt::Checked) global_com_instance->legacy_transfer_clock_flags(0, 1);
    else global_com_instance->legacy_transfer_clock_flags(0, 0);
}

void FluorescenceApp::on_lsettings_td_l0_enable_stateChanged(int arg1)
{
    if(arg1 == Qt::Checked) global_com_instance->legacy_transfer_clock_flags(3, 1);
    else global_com_instance->legacy_transfer_clock_flags(3, 0);
}

void FluorescenceApp::on_lsettings_td_intldate_enable_stateChanged(int arg1)
{
    if(arg1 == Qt::Checked) global_com_instance->legacy_transfer_clock_flags(1, 1);
    else global_com_instance->legacy_transfer_clock_flags(1, 0);
}

void FluorescenceApp::on_lsettings_nsh_enable_stateChanged(int arg1)
{
    vfdco_time_t dummy_time = {0, 0, 0};
    if(arg1 == Qt::Checked) global_com_instance->legacy_transfer_night_shift(dummy_time, dummy_time, 42, 1);
    else global_com_instance->legacy_transfer_night_shift(dummy_time, dummy_time, 42, 0);
}


void FluorescenceApp::on_lsettings_nsh_scheduled_enable_stateChanged(int arg1)
{
    vfdco_time_t t_start = {
        .h = (uint8_t)ui->settings_nsh_start->time().hour(),
        .m = (uint8_t)ui->settings_nsh_start->time().minute(),
        .s = 0
    };
    vfdco_time_t t_stop = {
        .h = (uint8_t)ui->settings_nsh_stop->time().hour(),
        .m = (uint8_t)ui->settings_nsh_stop->time().minute(),
        .s = 0
    };
    if(arg1 == Qt::Checked) global_com_instance->legacy_transfer_night_shift(t_start, t_stop, 1, 42);
    else global_com_instance->legacy_transfer_night_shift(t_start, t_stop, 0, 42);
}

void FluorescenceApp::on_lsettings_settings_default_clicked() { on_settings_settings_default_clicked(); }
void FluorescenceApp::on_lsettings_settings_save_clicked() { on_settings_settings_save_clicked(); }
void FluorescenceApp::on_lsettings_info_download_clicked() { on_settings_info_download_clicked(); }

void FluorescenceApp::on_timesync_button_clicked()
{
    QTime ct = QTime::currentTime();
    QDate cd = QDate::currentDate();
    vfdco_time_t v_time = {
        .h = (uint8_t)ct.hour(),
        .m = (uint8_t)ct.minute(),
        .s = (uint8_t)ct.second()
    };
    vfdco_date_t v_date = {
        .y = (uint8_t)(cd.year() % 100),
        .m = (uint8_t)cd.month(),
        .d = (uint8_t)cd.day()
    };
    global_com_instance->transfer_time_date(v_time, v_date);
}

void FluorescenceApp::on_static_off_clicked() {         global_com_instance->transfer_light_pattern(0, 0, 0); }
void FluorescenceApp::on_static_white_clicked() {       global_com_instance->transfer_light_pattern(0, 1, 0); }
void FluorescenceApp::on_static_warmwhite_clicked() {   global_com_instance->transfer_light_pattern(0, 2, 0); }
void FluorescenceApp::on_static_red_clicked() {         global_com_instance->transfer_light_pattern(0, 3, 0); }
void FluorescenceApp::on_static_green_clicked() {       global_com_instance->transfer_light_pattern(0, 4, 0); }
void FluorescenceApp::on_static_blue_clicked() {        global_com_instance->transfer_light_pattern(0, 5, 0); }
void FluorescenceApp::on_static_yellow_clicked() {      global_com_instance->transfer_light_pattern(0, 6, 0); }
void FluorescenceApp::on_static_orange_clicked() {      global_com_instance->transfer_light_pattern(0, 7, 0); }
void FluorescenceApp::on_static_cyan_clicked() {        global_com_instance->transfer_light_pattern(0, 8, 0); }
void FluorescenceApp::on_static_magenta_clicked() {     global_com_instance->transfer_light_pattern(0, 9, 0); }
void FluorescenceApp::on_static_violet_clicked() {      global_com_instance->transfer_light_pattern(0, 10, 0); }
void FluorescenceApp::on_static_rainbow_clicked() {     global_com_instance->transfer_light_pattern(0, 11, 0); }
void FluorescenceApp::on_static_pastelrainbow_clicked(){global_com_instance->transfer_light_pattern(0, 12, 0); }
void FluorescenceApp::on_static_green2blue_clicked() {  global_com_instance->transfer_light_pattern(0, 13, 0); }
void FluorescenceApp::on_static_red2blue_clicked() {    global_com_instance->transfer_light_pattern(0, 14, 0); }
void FluorescenceApp::on_static_red2green_clicked() {   global_com_instance->transfer_light_pattern(0, 15, 0); }

void FluorescenceApp::on_panel_dyn_spectrum_set_clicked()
{
    QString param1 = ui->panel_dyn_spectrum_param_1->currentText();
    QString param2 = ui->panel_dyn_spectrum_param_2->currentText();
    uint8_t param1_value, param2_value;
    // Sat
    if(param1 == "low") param1_value = 127;
    else if(param1 == "medium") param1_value = 196;
    else param1_value = 255;
    // Li
    if(param1 == "low") param2_value = 50;
    else if(param1 == "medium") param2_value = 90;
    else param2_value = 127;

    global_com_instance->transfer_light_pattern(1, param1_value, param2_value);
}

void FluorescenceApp::on_panel_dyn_bliss_set_clicked()
{
    // Direct index parsing
    uint8_t param1 = ui->panel_dyn_bliss_param->currentIndex();
    global_com_instance->transfer_light_pattern(2, param1, 0);
}

void FluorescenceApp::on_panel_dyn_rnb_set_clicked()
{
    uint8_t param1_value = (uint8_t)ui->panel_dyn_rnb_param1->value();
    QString param2 = ui->panel_dyn_rnb_param2->currentText();
    uint8_t param2_value;
    // Sat
    if(param2 == "low") param2_value = 127;
    else if(param2 == "medium") param2_value = 196;
    else param2_value = 255;
    global_com_instance->transfer_light_pattern(3, param1_value, param2_value);
}

void FluorescenceApp::on_panel_dyn_chase_set_clicked()
{
    uint8_t param1_value = (uint8_t)ui->panel_dyn_chase_param1->value();
    QString param2 = ui->panel_dyn_chase_param2->currentText();
    uint8_t param2_value;
    // Sat
    if(param2 == "low") param2_value = 127;
    else if(param2 == "medium") param2_value = 196;
    else param2_value = 255;
    global_com_instance->transfer_light_pattern(4, param1_value, param2_value);
}

void FluorescenceApp::on_panel_dyn_tcode_set_clicked()
{
    // Direct index parsing
    uint8_t param1 = ui->panel_dyn_tcode_param1->currentIndex();
    global_com_instance->transfer_light_pattern(5, param1, 0);
}

void FluorescenceApp::on_panel_dyn_cop_set_clicked()
{
    // Direct index parsing
    uint8_t param1 = ui->panel_dyn_cop_param1->currentIndex();
    global_com_instance->transfer_light_pattern(6, param1, 0);
}

void FluorescenceApp::on_panel_dyn_music_set_clicked()
{
    uint8_t param1_value = (uint8_t)ui->panel_dyn_music_param1->value();
    QString param2 = ui->panel_dyn_music_param2->currentText();
    uint8_t param2_value;
    // Sat
    if(param2 == "low") param2_value = 127;
    else if(param2 == "medium") param2_value = 196;
    else param2_value = 255;
    global_com_instance->transfer_light_pattern(7, param1_value, param2_value);
}


void FluorescenceApp::on_message_send_clicked()
{
    QString message = ui->message_text->toPlainText();
    uint8_t *m[4] = {NULL, NULL, NULL, NULL};

    if(message.length() == 0) {
        QMessageBox err_zero;
        err_zero.setText("Oops, u forgot the message, did ya?");
        err_zero.setIcon(QMessageBox::Information);
        err_zero.exec();
        return;
    }
    if(ui->message_set_custom_welcome->checkState() == Qt::Checked) {
        if(message.length() == 6) {
            uint8_t welcome_msg[6];
            for(uint_fast8_t i = 0; i < 6; ++i) welcome_msg[i] = message.toLatin1().data()[i];
            global_com_instance->transfer_welcome_set(welcome_msg);
        } else {
            QMessageBox err_non6;
            err_non6.setText("Oops, the welcome message needs exactly 6 characters!");
            err_non6.setIcon(QMessageBox::Information);
            err_non6.exec();
            return;
        }
    } else {
        if(message.length() > 4 * 6) {
            QMessageBox err_too_long;
            err_too_long.setText("Oops, the message is longer than 4 * 6 = 24 characters.\n Please try again with less.");
            err_too_long.setIcon(QMessageBox::Information);
            err_too_long.exec();
            return;
        }
        uint_fast8_t n_iterations = message.length() / 6;
        uint_fast8_t remainder = message.length() % 6;
        uint_fast8_t i;
        for(i = 0; i < n_iterations; ++i) {
            m[i] = (uint8_t *)calloc(6, sizeof(uint8_t));
            QString message_substr = message.mid(i * 6, 6);
            for(uint_fast8_t j = 0; j < message_substr.length(); ++j) m[i][j] = message_substr.at(j).toLatin1();
        }
        if(remainder) {
            m[i] = (uint8_t *)calloc(6, sizeof(uint8_t));
            QString message_substr = message.mid(i * 6, remainder);
            for(uint_fast8_t j = 0; j < message_substr.length(); ++j) m[i][j] = message_substr.at(j).toLatin1();
        }
        global_com_instance->transfer_message(m[0], m[1], m[2], m[3], (uint8_t)ui->message_length->toPlainText().toUShort());
        for(uint_fast8_t i = 0; i < 4; ++i) {
            if(m[i]) free(m[i]);
        }
    }
}