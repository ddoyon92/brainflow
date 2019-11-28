﻿using System.Runtime.InteropServices;

namespace brainflow
{
    public enum FilterTypes
    {
        BUTTERWORTH = 0,
        CHEBYSHEV_TYPE_1 = 1,
        BESSEL = 2
    };

    public enum AggOperations
    {
        MEAN = 0,
        MEDIAN = 1,
        EACH = 2
    };

    class DataHandlerLibrary64
    {
        [DllImport ("DataHandler.dll", SetLastError = true, CallingConvention = CallingConvention.Cdecl)]
        public static extern int perform_lowpass (double[] data, int len, int sampling_rate, double cutoff, int order, int filter_type, double ripple);
        [DllImport ("DataHandler.dll", SetLastError = true, CallingConvention = CallingConvention.Cdecl)]
        public static extern int perform_highpass (double[] data, int len, int sampling_rate, double cutoff, int order, int filter_type, double ripple);
        [DllImport ("DataHandler.dll", SetLastError = true, CallingConvention = CallingConvention.Cdecl)]
        public static extern int perform_bandpass (double[] data, int len, int sampling_rate, double center_freq, double band_width, int order, int filter_type, double ripple);
        [DllImport ("DataHandler.dll", SetLastError = true, CallingConvention = CallingConvention.Cdecl)]
        public static extern int perform_bandstop (double[] data, int len, int sampling_rate, double center_freq, double band_width, int order, int filter_type, double ripple);
        [DllImport ("DataHandler.dll", SetLastError = true, CallingConvention = CallingConvention.Cdecl)]
        public static extern int perform_rolling_filter (double[] data, int len, int period, int operation);
        [DllImport ("DataHandler.dll", SetLastError = true, CallingConvention = CallingConvention.Cdecl)]
        public static extern int perform_downsampling (double[] data, int len, int period, int operation, double[] downsampled_data);
        [DllImport ("DataHandler.dll", SetLastError = true, CallingConvention = CallingConvention.Cdecl)]
        public static extern int write_file (double[] data, int num_rows, int num_cols, string file_name, string file_mode);
        [DllImport ("DataHandler.dll", SetLastError = true, CallingConvention = CallingConvention.Cdecl)]
        public static extern int read_file (double[] data, int[] num_rows, int[] num_cols, string file_name, int max_elements);
        [DllImport ("DataHandler.dll", SetLastError = true, CallingConvention = CallingConvention.Cdecl)]
        public static extern int get_num_elements_in_file (string file_name, int[] num_elements);
        [DllImport ("DataHandler.dll", SetLastError = true, CallingConvention = CallingConvention.Cdecl)]
        public static extern int perform_wavelet_transform (double[] data, int data_len, string wavelet, int decomposition_level, double[] output_data, int[] decomposition_lengths);
        [DllImport ("DataHandler.dll", SetLastError = true, CallingConvention = CallingConvention.Cdecl)]
        public static extern int perform_inverse_wavelet_transform (double[] wavelet_coeffs, int original_data_len, string wavelet, int decomposition_level,
                                                                    int[] decomposition_lengths, double[] output_data);
        [DllImport ("DataHandler.dll", SetLastError = true, CallingConvention = CallingConvention.Cdecl)]
        public static extern int perform_wavelet_denoising (double[] data, int data_len, string wavelet, int decomposition_level);

    }

    class DataHandlerLibrary32
    {
        [DllImport ("DataHandler32.dll", SetLastError = true, CallingConvention = CallingConvention.Cdecl)]
        public static extern int perform_lowpass (double[] data, int len, int sampling_rate, double cutoff, int order, int filter_type, double ripple);
        [DllImport ("DataHandler32.dll", SetLastError = true, CallingConvention = CallingConvention.Cdecl)]
        public static extern int perform_highpass (double[] data, int len, int sampling_rate, double cutoff, int order, int filter_type, double ripple);
        [DllImport ("DataHandler32.dll", SetLastError = true, CallingConvention = CallingConvention.Cdecl)]
        public static extern int perform_bandpass (double[] data, int len, int sampling_rate, double center_freq, double band_width, int order, int filter_type, double ripple);
        [DllImport ("DataHandler32.dll", SetLastError = true, CallingConvention = CallingConvention.Cdecl)]
        public static extern int perform_bandstop (double[] data, int len, int sampling_rate, double center_freq, double band_width, int order, int filter_type, double ripple);
        [DllImport ("DataHandler32.dll", SetLastError = true, CallingConvention = CallingConvention.Cdecl)]
        public static extern int perform_rolling_filter (double[] data, int len, int period, int operation);
        [DllImport ("DataHandler32.dll", SetLastError = true, CallingConvention = CallingConvention.Cdecl)]
        public static extern int perform_downsampling (double[] data, int len, int period, int operation, double[] downsampled_data);
        [DllImport ("DataHandler32.dll", SetLastError = true, CallingConvention = CallingConvention.Cdecl)]
        public static extern int write_file (double[] data, int num_rows, int num_cols, string file_name, string file_mode);
        [DllImport ("DataHandler32.dll", SetLastError = true, CallingConvention = CallingConvention.Cdecl)]
        public static extern int read_file (double[] data, int[] num_rows, int[] num_cols, string file_name, int max_elements);
        [DllImport ("DataHandler32.dll", SetLastError = true, CallingConvention = CallingConvention.Cdecl)]
        public static extern int get_num_elements_in_file (string file_name, int[] num_elements);
        [DllImport ("DataHandler32.dll", SetLastError = true, CallingConvention = CallingConvention.Cdecl)]
        public static extern int perform_wavelet_transform (double[] data, int data_len, string wavelet, int decomposition_level, double[] output_data, int[] decomposition_lengths);
        [DllImport ("DataHandler32.dll", SetLastError = true, CallingConvention = CallingConvention.Cdecl)]
        public static extern int perform_inverse_wavelet_transform (double[] wavelet_coeffs, int original_data_len, string wavelet, int decomposition_level,
                                                                    int[] decomposition_lengths, double[] output_data);
        [DllImport ("DataHandler32.dll", SetLastError = true, CallingConvention = CallingConvention.Cdecl)]
        public static extern int perform_wavelet_denoising (double[] data, int data_len, string wavelet, int decomposition_level);
    }

    class DataHandlerLibrary
    {
        public static int perform_lowpass (double[] data, int len, int sampling_rate, double cutoff, int order, int filter_type, double ripple)
        {
            if (System.Environment.Is64BitProcess)
                return DataHandlerLibrary64.perform_lowpass (data, len, sampling_rate, cutoff, order, filter_type, ripple);
            else
                return DataHandlerLibrary32.perform_lowpass (data, len, sampling_rate, cutoff, order, filter_type, ripple);
        }

        public static int perform_highpass (double[] data, int len, int sampling_rate, double cutoff, int order, int filter_type, double ripple)
        {
            if (System.Environment.Is64BitProcess)
                return DataHandlerLibrary64.perform_highpass (data, len, sampling_rate, cutoff, order, filter_type, ripple);
            else
                return DataHandlerLibrary32.perform_highpass (data, len, sampling_rate, cutoff, order, filter_type, ripple);
        }

        public static int perform_bandpass (double[] data, int len, int sampling_rate, double center_freq, double band_width, int order, int filter_type, double ripple)
        {
            if (System.Environment.Is64BitProcess)
                return DataHandlerLibrary64.perform_bandpass (data, len, sampling_rate, center_freq, band_width, order, filter_type, ripple);
            else
                return DataHandlerLibrary32.perform_bandpass (data, len, sampling_rate, center_freq, band_width, order, filter_type, ripple);
        }

        public static int perform_bandstop (double[] data, int len, int sampling_rate, double center_freq, double band_width, int order, int filter_type, double ripple)
        {
            if (System.Environment.Is64BitProcess)
                return DataHandlerLibrary64.perform_bandstop (data, len, sampling_rate, center_freq, band_width, order, filter_type, ripple);
            else
                return DataHandlerLibrary32.perform_bandstop (data, len, sampling_rate, center_freq, band_width, order, filter_type, ripple);
        }

        public static int perform_rolling_filter (double[] data, int len, int period, int operation)
        {
            if (System.Environment.Is64BitProcess)
                return DataHandlerLibrary64.perform_rolling_filter (data, len, period, operation);
            else
                return DataHandlerLibrary32.perform_rolling_filter (data, len, period, operation);
        }

        public static int perform_downsampling (double[] data, int len, int period, int operation, double[] filtered_data)
        {
            if (System.Environment.Is64BitProcess)
                return DataHandlerLibrary64.perform_downsampling (data, len, period, operation, filtered_data);
            else
                return DataHandlerLibrary32.perform_downsampling (data, len, period, operation, filtered_data);
        }

        public static int read_file (double[] data, int[] num_rows, int[] num_cols, string file_name, int max_elements)
        {
            if (System.Environment.Is64BitProcess)
                return DataHandlerLibrary64.read_file (data, num_rows, num_cols, file_name, max_elements);
            else
                return DataHandlerLibrary32.read_file (data, num_rows, num_cols, file_name, max_elements);
        }

        public static int write_file (double[] data, int num_rows, int num_cols, string file_name, string file_mode)
        {
            if (System.Environment.Is64BitProcess)
                return DataHandlerLibrary64.write_file (data, num_rows, num_cols, file_name, file_mode);
            else
                return DataHandlerLibrary32.write_file (data, num_rows, num_cols, file_name, file_mode);
        }

        public static int get_num_elements_in_file (string file_name, int[] num_elements)
        {
            if (System.Environment.Is64BitProcess)
                return DataHandlerLibrary64.get_num_elements_in_file (file_name, num_elements);
            else
                return DataHandlerLibrary32.get_num_elements_in_file (file_name, num_elements);
        }

        public static int perform_wavelet_transform (double[] data, int data_len, string wavelet, int decomposition_level, double[] output_data, int[] decomposition_lengths)
        {
            if (System.Environment.Is64BitProcess)
                return DataHandlerLibrary64.perform_wavelet_transform (data, data_len, wavelet, decomposition_level, output_data, decomposition_lengths);
            else
                return DataHandlerLibrary32.perform_wavelet_transform (data, data_len, wavelet, decomposition_level, output_data, decomposition_lengths);
        }

        public static int perform_inverse_wavelet_transform (double[] wavelet_coeffs, int original_data_len, string wavelet, int decomposition_level,
                                                                    int[] decomposition_lengths, double[] output_data)
        {
            if (System.Environment.Is64BitProcess)
                return DataHandlerLibrary64.perform_inverse_wavelet_transform (wavelet_coeffs, original_data_len, wavelet, decomposition_level, decomposition_lengths, output_data);
            else
                return DataHandlerLibrary32.perform_inverse_wavelet_transform (wavelet_coeffs, original_data_len, wavelet, decomposition_level, decomposition_lengths, output_data);
        }

        public static int perform_wavelet_denoising (double[] data, int data_len, string wavelet, int decomposition_level)
        {
            if (System.Environment.Is64BitProcess)
                return DataHandlerLibrary64.perform_wavelet_denoising (data, data_len, wavelet, decomposition_level);
            else
                return DataHandlerLibrary32.perform_wavelet_denoising (data, data_len, wavelet, decomposition_level);
        }
    }
}
