#ifndef FLUORESCENCEAPP_H
#define FLUORESCENCEAPP_H

#include <QMainWindow>
#include "fl_app_com.h"

QT_BEGIN_NAMESPACE
namespace Ui { class FluorescenceApp; }
QT_END_NAMESPACE

#define NUM_PRESET_DYNAMIC_COLORS 6
#define NUM_PRESET_DYNAMIC_TIME 21

class FluorescenceApp : public QMainWindow
{
    Q_OBJECT

public:
    FluorescenceApp(QWidget *parent = nullptr);
    ~FluorescenceApp();
    void update();

private slots:

    void on_com_connect_clicked();

    void on_static_rainbow_clicked();

    void on_static_pastelrainbow_clicked();

    void on_main_preset_clicked();

    void on_main_custom_clicked();

    void on_custom_slider_r_sliderMoved(int position);

    void on_custom_slider_g_sliderMoved(int position);

    void on_custom_slider_b_sliderMoved(int position);

    void on_custom_value_r_valueChanged(int arg1);

    void on_custom_value_g_valueChanged(int arg1);

    void on_custom_value_b_valueChanged(int arg1);

    void on_custom_value_w_valueChanged(int arg1);

    void on_custom_value_h_valueChanged(int arg1);

    void on_custom_value_s_valueChanged(int arg1);

    void on_custom_value_l_valueChanged(int arg1);

    void on_custom_slider_h_sliderMoved(int position);

    void on_custom_slider_s_sliderMoved(int position);

    void on_custom_slider_l_sliderMoved(int position);

    void on_custom_slider_w_sliderMoved(int position);

    void on_custom_value_hex_colorEditingFinished(const QColor &color);

    void on_main_timesync_clicked();

    void on_main_message_clicked();

    void on_custom_color_wheel_colorSelected(const QColor &arg1);

    void on_dynamic_spectrum_clicked();

    void on_dynamic_bliss_dnc_clicked();

    void on_dynamic_rainbow_clicked();

    void on_dynamic_chase_clicked();

    void on_dynamic_timecode_clicked();

    void on_dynamic_police_clicked();

    void on_panel_dyn_bliss_param_currentIndexChanged(int index);

    void on_main_settings_clicked();

    void on_settings_td_set_clicked();

    void on_settings_bri_set_clicked();

    void on_settings_nsh_enable_stateChanged(int arg1);

    void on_settings_nsh_set_clicked();

    void on_settings_settings_default_clicked();

    void on_settings_settings_save_clicked();

    void on_settings_info_download_clicked();

    void on_lsettings_td_12hr_enable_stateChanged(int arg1);

    void on_lsettings_td_l0_enable_stateChanged(int arg1);

    void on_lsettings_td_intldate_enable_stateChanged(int arg1);

    void on_lsettings_nsh_enable_stateChanged(int arg1);

    void on_lsettings_nsh_scheduled_enable_stateChanged(int arg1);

    void on_lsettings_settings_default_clicked();

    void on_lsettings_settings_save_clicked();

    void on_lsettings_info_download_clicked();

    void on_timesync_button_clicked();

    void on_static_off_clicked();

    void on_static_white_clicked();

    void on_static_red_clicked();

    void on_static_warmwhite_clicked();

    void on_static_green_clicked();

    void on_static_blue_clicked();

    void on_static_yellow_clicked();

    void on_static_orange_clicked();

    void on_static_cyan_clicked();

    void on_static_magenta_clicked();

    void on_static_violet_clicked();

    void on_static_green2blue_clicked();

    void on_static_red2blue_clicked();

    void on_static_red2green_clicked();

    void on_panel_dyn_spectrum_set_clicked();

    void on_panel_dyn_bliss_set_clicked();

    void on_panel_dyn_rnb_set_clicked();

    void on_panel_dyn_chase_set_clicked();

    void on_panel_dyn_tcode_set_clicked();

    void on_panel_dyn_cop_set_clicked();

    void on_panel_dyn_music_set_clicked();

    void on_message_send_clicked();

private:
    Ui::FluorescenceApp *ui;

    fl_app_com *global_com_instance = nullptr;

    QColor custom_global_color;
    QColor custom_led_colors[6];
    uint8_t custom_global_color_white;
    uint8_t custom_led_white[6];

    void hide_all_panels();
    void hide_all_dynamic_control_panels();
    void clear_lights_instance();
    void custom_color_update_all_sliders(bool);
};
#endif // FLUORESCENCEAPP_H