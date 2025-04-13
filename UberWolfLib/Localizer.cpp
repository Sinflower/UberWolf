/*
 *  File: Localizer.cpp
 *  Copyright (c) 2024 Sinflower
 *
 *  MIT License
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 */

#include "Localizer.h"

#include <windows.h>

namespace uberWolfLib
{
const tString& Localizer::GetValueT(const std::string& key) const
{
	if (m_localizationFunction)
		return m_localizationFunction(key);

	if (s_localizationT.find(key) != s_localizationT.end())
		return s_localizationT.at(key);
	else
		return s_errorStringT;
}

Localizer::LocMapT Localizer::s_localizationT = {
	{ "dec_key_search_msg", TEXT("Searching for decryption key ...") },
	{ "unpacked_msg", TEXT("{} is already unpacked, skipping") },
	{ "unpacking_msg", TEXT("Unpacking: {} ... ") },
	{ "done_msg", TEXT("Done") },
	{ "failed_msg", TEXT("Failed") },
	{ "pro_game_detected_msg", TEXT("WolfPro game detected, trying to get decryption key ...") },
	{ "det_key_error_msg", TEXT("Failed to find the decryption key") },
	{ "det_key_found_msg", TEXT("Found the decryption key, restarting extraction") },
	{ "det_key_inj_msg", TEXT("Trying to get decryption key using injection ... ") },
	{ "exec_game_inj_msg", TEXT("Executing game and injecting DLL ... ") },
	{ "inj_error_msg", TEXT("Injecting the DLL failed, stopping") },
	{ "search_game_msg", TEXT("Searching for game executable in: {}") },
	{ "exe_found_msg", TEXT("Found game executable: {}") },
	{ "exe_error_msg", TEXT("Could not find the game executable") },
	{ "dll_copy_msg", TEXT("Copying DLL from resource to {} ...") },
	{ "dll_copied_msg", TEXT("DLL copied successfully") },
	{ "dll_error_msg_1", TEXT("UberWolfLib: Failed to find resource") },
	{ "dll_error_msg_2", TEXT("UberWolfLib: Failed to load resource") },
	{ "dll_error_msg_3", TEXT("UberWolfLib: Failed to create file") },
	{ "key_file_warn_msg", TEXT("WARNING: Unable to find DxArc key file, this does not look like a WolfPro game") },
	{ "inv_prot_key_error_msg", TEXT("ERROR: Invalid protection key") },
	{ "dxarc_key_error_msg", TEXT("ERROR: Unable to find DxArc key") },
	{ "remove_prot_error_msg", TEXT("ERROR: Unable to remove protection, this does not look like a WolfPro game") },
	{ "data_dir_error_msg", TEXT("ERROR: Unable to remove protection, data folder not set") },
	{ "unprot_dir_create_error_msg", TEXT("ERROR: Unable to create unprotected folder ({})") },
	{ "decrypt_key_error_msg", TEXT("ERROR: Unable to decrypt protection key file") },
	{ "prot_key_len_error_msg", TEXT("ERROR: Invalid key length, exiting ...") },
	{ "open_file_error_msg", TEXT("ERROR: Unable to open file \"{}\".") },
	{ "get_file_size_error_msg", TEXT("ERROR: Unable to get file size for \"{}\".") },
	{ "read_file_error_msg", TEXT("ERROR: Unable to read file \"{}\".") },
	{ "write_file_error_msg", TEXT("ERROR: Unable to write file \"{}\".") },
	{ "find_file_error_msg", TEXT("ERROR: Unable to find file \"{}\".") },
	{ "decrypt_error_msg", TEXT("ERROR: Unable to decrypt protected file") },
	{ "unprot_file_loc", TEXT("Unprotected files can be found in: {}") },
	{ "remove_prot", TEXT("Removing protection from: {} ... ") },
	{ "calc_prot_key_error_msg", TEXT("ERROR: Unable to calculate protection key") }
};

tString Localizer::s_errorStringT = TEXT("NO DEFAULT FOUND");
}; // namespace uberWolfLib