#include <numeric>
#include <stdint.h>
#include <string.h>

#include "custom_cast.h"
#include "galea_serial.h"
#include "timestamp.h"

#include "json.hpp"

using json = nlohmann::json;

#ifndef _WIN32
#include <errno.h>
#endif


constexpr int GaleaSerial::base_package_size;
constexpr int GaleaSerial::exg_package_size;
constexpr int GaleaSerial::num_base_packages;
constexpr int GaleaSerial::bytes_in_single_entry;
constexpr int GaleaSerial::num_exg_packages_per_base;
constexpr int GaleaSerial::transaction_size;
constexpr unsigned char GaleaSerial::start_byte;
constexpr unsigned char GaleaSerial::stop_byte;


GaleaSerial::GaleaSerial (struct BrainFlowInputParams params)
    : Board ((int)BoardIds::GALEA_SERIAL_BOARD, params)
{
    serial = NULL;
    is_streaming = false;
    keep_alive = false;
    initialized = false;
    state = (int)BrainFlowExitCodes::SYNC_TIMEOUT_ERROR;
    half_rtt = 0.0;
}

GaleaSerial::~GaleaSerial ()
{
    skip_logs = true;
    release_session ();
}

int GaleaSerial::prepare_session ()
{
    // check params
    if (initialized)
    {
        safe_logger (spdlog::level::info, "Session is already prepared");
        return (int)BrainFlowExitCodes::STATUS_OK;
    }
    if (params.serial_port.empty ())
    {
        safe_logger (spdlog::level::err, "serial port is not specified.");
        return (int)BrainFlowExitCodes::INVALID_ARGUMENTS_ERROR;
    }
    if ((params.timeout > 600) || (params.timeout < 1))
    {
        params.timeout = 3;
    }

    // create serial object
    serial = Serial::create (params.serial_port.c_str (), this);
    int res = serial->open_serial_port ();
    if (res < 0)
    {
        safe_logger (spdlog::level::err,
            "Make sure you provided correct port name and have permissions to open it(run with "
            "sudo/admin). Also, close all other apps using this port.");
        return (int)BrainFlowExitCodes::UNABLE_TO_OPEN_PORT_ERROR;
    }
    res = serial->set_serial_port_settings (params.timeout * 1000, false);
    if (res < 0)
    {
        safe_logger (spdlog::level::err, "Unable to set port settings, res is {}", res);
        delete serial;
        serial = NULL;
        return (int)BrainFlowExitCodes::SET_PORT_ERROR;
    }
    res = serial->set_custom_baudrate (921600);
    if (res < 0)
    {
        safe_logger (spdlog::level::err, "Unable to set custom baud rate, res is {}", res);
        delete serial;
        serial = NULL;
        return (int)BrainFlowExitCodes::SET_PORT_ERROR;
    }
    safe_logger (spdlog::level::trace, "set port settings");

    // set initial settings
    std::string tmp;
    std::string default_settings = "o"; // use demo mode with agnd
    res = config_board (default_settings, tmp);
    if (res != (int)BrainFlowExitCodes::STATUS_OK)
    {
        safe_logger (spdlog::level::err, "failed to apply default settings");
        delete serial;
        serial = NULL;
        return (int)BrainFlowExitCodes::BOARD_NOT_READY_ERROR;
    }
    std::string sampl_rate = "~6";
    res = config_board (sampl_rate, tmp);
    if (res != (int)BrainFlowExitCodes::STATUS_OK)
    {
        safe_logger (spdlog::level::err, "failed to apply defaul sampling rate");
        delete serial;
        serial = NULL;
        return (int)BrainFlowExitCodes::BOARD_NOT_READY_ERROR;
    }
    initialized = true;

    return (int)BrainFlowExitCodes::STATUS_OK;
}

int GaleaSerial::config_board (std::string conf, std::string &response)
{
    if (serial == NULL)
    {
        safe_logger (spdlog::level::err, "You need to call prepare_session before config_board");
        return (int)BrainFlowExitCodes::BOARD_NOT_CREATED_ERROR;
    }
    // special handling for calc_time command
    if (conf == "calc_time")
    {
        if (is_streaming)
        {
            safe_logger (spdlog::level::err, "can not calc delay during the streaming.");
            return (int)BrainFlowExitCodes::BOARD_NOT_CREATED_ERROR;
        }
        int res = calc_time (response);
        return res;
    }

    std::string new_conf = conf + "\n";
    int len = (int)new_conf.size ();
    safe_logger (spdlog::level::debug, "Trying to config GaleaSerial with {}", new_conf.c_str ());
    int res = serial->send_to_serial_port (new_conf.c_str (), len);
    if (len != res)
    {
        safe_logger (spdlog::level::err, "Failed to config a board");
        return (int)BrainFlowExitCodes::BOARD_WRITE_ERROR;
    }

    return (int)BrainFlowExitCodes::STATUS_OK;
}

int GaleaSerial::start_stream (int buffer_size, char *streamer_params)
{
    if (!initialized)
    {
        safe_logger (spdlog::level::err, "You need to call prepare_session before config_board");
        return (int)BrainFlowExitCodes::BOARD_NOT_CREATED_ERROR;
    }
    if (is_streaming)
    {
        safe_logger (spdlog::level::err, "Streaming thread already running");
        return (int)BrainFlowExitCodes::STREAM_ALREADY_RUN_ERROR;
    }

    // calc time before start stream
    std::string resp;
    for (int i = 0; i < 3; i++)
    {
        int res = calc_time (resp);
        if (res != (int)BrainFlowExitCodes::STATUS_OK)
        {
            return res;
        }
    }

    int res = prepare_for_acquisition (buffer_size, streamer_params);
    if (res != (int)BrainFlowExitCodes::STATUS_OK)
    {
        return res;
    }

    // start streaming
    res = serial->send_to_serial_port ("b\n", 2);
    if (res != 2)
    {
        safe_logger (spdlog::level::err, "Failed to send a command to board");
        return (int)BrainFlowExitCodes::BOARD_WRITE_ERROR;
    }

    keep_alive = true;
    streaming_thread = std::thread ([this] { this->read_thread (); });
    // wait for data to ensure that everything is okay
    std::unique_lock<std::mutex> lk (this->m);
    auto sec = std::chrono::seconds (1);
    if (cv.wait_for (lk, 3 * sec,
            [this] { return this->state != (int)BrainFlowExitCodes::SYNC_TIMEOUT_ERROR; }))
    {
        this->is_streaming = true;
        return this->state;
    }
    else
    {
        safe_logger (spdlog::level::err, "no data received in 5sec, stopping thread");
        this->is_streaming = true;
        this->stop_stream ();
        return (int)BrainFlowExitCodes::SYNC_TIMEOUT_ERROR;
    }
}

int GaleaSerial::stop_stream ()
{
    if (is_streaming)
    {
        keep_alive = false;
        is_streaming = false;
        streaming_thread.join ();
        this->state = (int)BrainFlowExitCodes::SYNC_TIMEOUT_ERROR;
        int res = serial->send_to_serial_port ("s\n", 2);
        if (res != 2)
        {
            safe_logger (spdlog::level::err, "Failed to send a command to board");
            return (int)BrainFlowExitCodes::BOARD_WRITE_ERROR;
        }

        // free kernel buffer
        unsigned char b;
        res = 1;
        int max_attempt = 40000; // to dont get to infinite loop
        int current_attempt = 0;
        while (res == 1)
        {
            res = serial->read_from_serial_port (&b, 1);
            current_attempt++;
            if (current_attempt == max_attempt)
            {
                safe_logger (
                    spdlog::level::err, "Command 's' was sent but streaming is still running.");
                return (int)BrainFlowExitCodes::BOARD_WRITE_ERROR;
            }
        }

        std::string resp;
        for (int i = 0; i < 3; i++)
        {
            res = calc_time (resp); // call it in the end once to print time in the end
            if (res != (int)BrainFlowExitCodes::STATUS_OK)
            {
                break; // dont send exit code
            }
        }
        return (int)BrainFlowExitCodes::STATUS_OK;
    }
    else
    {
        return (int)BrainFlowExitCodes::STREAM_THREAD_IS_NOT_RUNNING;
    }
}

int GaleaSerial::release_session ()
{
    if (initialized)
    {
        if (is_streaming)
        {
            stop_stream ();
        }
        free_packages ();
        initialized = false;
        if (serial)
        {
            delete serial;
            serial = NULL;
        }
    }
    return (int)BrainFlowExitCodes::STATUS_OK;
}

void GaleaSerial::read_thread ()
{
    int res = 0;
    unsigned char b[GaleaSerial::transaction_size];
    DataBuffer time_buffer (1, 11);
    double latest_times[10];
    constexpr int offset_last_base_package =
        GaleaSerial::bytes_in_single_entry * (GaleaSerial::num_base_packages - 1);
    for (int i = 0; i < GaleaSerial::transaction_size; i++)
    {
        b[i] = 0;
    }
    int num_rows = board_descr["num_rows"];
    double *package = new double[num_rows];
    for (int i = 0; i < num_rows; i++)
    {
        package[i] = 0.0;
    }

    while (keep_alive)
    {
        // read and check first byte
        res = serial->read_from_serial_port (b, 1);
        if (res != 1)
        {
            safe_logger (spdlog::level::debug, "unable to read 1 byte");
            continue;
        }
        double pc_timestamp = get_timestamp ();
        if (b[0] != GaleaSerial::start_byte)
        {
            continue;
        }
        // read and check reamining bytes
        int remaining_bytes = GaleaSerial::transaction_size;
        int pos = 0;
        while ((remaining_bytes > 0) && (keep_alive))
        {
            res = serial->read_from_serial_port (b + pos, remaining_bytes);
            remaining_bytes -= res;
            pos += res;
        }
        if (!keep_alive)
        {
            break;
        }
        unsigned char last_byte = 0;
        res = serial->read_from_serial_port (&last_byte, 1);
        if (res != 1)
        {
            safe_logger (spdlog::level::debug, "Failed to read last byte");
            continue;
        }
        if (last_byte != GaleaSerial::stop_byte)
        {
            safe_logger (spdlog::level::debug, "Wrong end byte {}", last_byte);
            continue;
        }
        else
        {
            if (this->state != (int)BrainFlowExitCodes::STATUS_OK)
            {
                safe_logger (spdlog::level::info,
                    "received first package with {} bytes streaming is started");
                {
                    std::lock_guard<std::mutex> lk (this->m);
                    this->state = (int)BrainFlowExitCodes::STATUS_OK;
                }
                this->cv.notify_one ();
                safe_logger (spdlog::level::debug, "start streaming");
            }
        }
        float timestamp_last_package = 0.0;
        memcpy (&timestamp_last_package, b + 64 + offset_last_base_package, 4);
        timestamp_last_package /= 1000; // from ms to seconds
        double time_delta = pc_timestamp - timestamp_last_package;
        time_buffer.add_data (&time_delta);
        int num_time_deltas = (int)time_buffer.get_current_data (10, latest_times);
        time_delta = 0.0;
        for (int i = 0; i < num_time_deltas; i++)
        {
            time_delta += latest_times[i];
        }
        time_delta /= num_time_deltas;

        // todo: avoid copypaste
        for (int cur_base_package = 0; cur_base_package < GaleaSerial::num_base_packages;
             cur_base_package++)
        {
            // parse full base package
            int offset = cur_base_package * GaleaSerial::bytes_in_single_entry;
            package[board_descr["package_num_channel"].get<int> ()] = (double)b[0 + offset];
            for (int i = 4, tmp_counter = 0; i < 20; i++, tmp_counter++)
            {
                if (tmp_counter < 6)
                    package[i - 3] =
                        emg_scale * (double)cast_24bit_to_int32 (b + offset + 5 + 3 * (i - 4));
                else if ((tmp_counter == 6) || (tmp_counter == 7)) // fp1 and fp2
                    package[i - 3] = eeg_scale_sister_board *
                        (double)cast_24bit_to_int32 (b + offset + 5 + 3 * tmp_counter);
                else
                    package[i - 3] = eeg_scale_main_board *
                        (double)cast_24bit_to_int32 (b + offset + 5 + 3 * tmp_counter);
            }
            uint16_t temperature;
            int32_t ppg_ir;
            int32_t ppg_red;
            float eda;
            memcpy (&temperature, b + 54 + offset, 2);
            memcpy (&eda, b + 1 + offset, 4);
            memcpy (&ppg_red, b + 56 + offset, 4);
            memcpy (&ppg_ir, b + 60 + offset, 4);
            package[board_descr["ppg_channels"][0].get<int> ()] = (double)ppg_red;
            package[board_descr["ppg_channels"][1].get<int> ()] = (double)ppg_ir;
            package[board_descr["eda_channels"][0].get<int> ()] = (double)eda;
            package[board_descr["temperature_channels"][0].get<int> ()] = temperature / 100.0;
            package[board_descr["battery_channel"].get<int> ()] = (double)b[53 + offset];

            float timestamp_device = 0.0;
            memcpy (&timestamp_device, b + 64 + offset, 4);
            timestamp_device /= 1000; // from ms to seconds

            package[board_descr["timestamp_channel"].get<int> ()] =
                timestamp_device + time_delta - half_rtt;
            package[board_descr["other_channels"][0].get<int> ()] = pc_timestamp;
            package[board_descr["other_channels"][1].get<int> ()] = (double)timestamp_device;

            push_package (package);

            // parse exg only packages
            for (int exg_package_num = 0; exg_package_num < GaleaSerial::num_exg_packages_per_base;
                 exg_package_num++)
            {
                int exg_offset = offset + GaleaSerial::exg_package_size * exg_package_num;
                for (int i = 4, tmp_counter = 0; i < 20; i++, tmp_counter++)
                {
                    if (tmp_counter < 6)
                        package[i - 3] = emg_scale *
                            (double)cast_24bit_to_int32 (b + exg_offset + 3 * tmp_counter);
                    else if ((tmp_counter == 6) || (tmp_counter == 11)) // fp1 and fp2
                        package[i - 3] = eeg_scale_sister_board *
                            (double)cast_24bit_to_int32 (b + exg_offset + 3 * tmp_counter);
                    else
                        package[i - 3] = eeg_scale_main_board *
                            (double)cast_24bit_to_int32 (b + exg_offset + 3 * tmp_counter);
                }
                timestamp_device = 0.0;
                memcpy (&timestamp_device, b + 48 + exg_offset, 4);
                timestamp_device /= 1000; // from ms to seconds
                package[board_descr["timestamp_channel"].get<int> ()] =
                    timestamp_device + time_delta - half_rtt;
                package[board_descr["other_channels"][0].get<int> ()] = pc_timestamp;
                package[board_descr["other_channels"][1].get<int> ()] = (double)timestamp_device;
                package[board_descr["package_num_channel"].get<int> ()]++;
                push_package (package);
            }
        }
    }
    delete[] package;
}

int GaleaSerial::calc_time (std::string &resp)
{
    constexpr int bytes_to_calc_rtt = 4;
    unsigned char b[bytes_to_calc_rtt];

    double start = get_timestamp ();
    int res = serial->send_to_serial_port ("F444\n", 5);
    if (res != 5)
    {
        safe_logger (spdlog::level::warn, "failed to send time calc command to device");
        return (int)BrainFlowExitCodes::BOARD_WRITE_ERROR;
    }
    res = serial->read_from_serial_port (b, bytes_to_calc_rtt);
    double done = get_timestamp ();
    if (res != bytes_to_calc_rtt)
    {
        safe_logger (
            spdlog::level::warn, "failed to recv resp from time calc command, resp size {}", res);
        return (int)BrainFlowExitCodes::BOARD_WRITE_ERROR;
    }
    double duration = done - start;
    float timestamp_device = 0;
    memcpy (&timestamp_device, b, 4);
    timestamp_device /= 1000;
    half_rtt = duration / 2;

    json result;
    result["rtt"] = duration;
    result["timestamp_device"] = (float)timestamp_device;
    result["pc_timestamp"] = start + half_rtt;

    resp = result.dump ();
    safe_logger (spdlog::level::info, "calc_time output: {}", resp);

    return (int)BrainFlowExitCodes::STATUS_OK;
}