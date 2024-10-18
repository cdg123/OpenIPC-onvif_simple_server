/*
 * Copyright (c) 2023 roleo.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>

#include "media_service.h"
#include "conf.h"
#include "fault.h"
#include "utils.h"
#include "log.h"
#include "ezxml_wrapper.h"
#include "onvif_simple_server.h"

extern service_context_t service_ctx;

int media_get_service_capabilities()
{
    long size = cat(NULL, "media_service_files/GetServiceCapabilities.xml", 0);

    fprintf(stdout, "Content-type: application/soap+xml\r\n");
    fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

    return cat("stdout", "media_service_files/GetServiceCapabilities.xml", 0);
}

int media_get_video_sources()
{
    // Get the video source from the 1st profile
    char stmp_w[16], stmp_h[16];

    sprintf(stmp_w, "%d", service_ctx.profiles[0].width);
    sprintf(stmp_h, "%d", service_ctx.profiles[0].height);
    long size = cat(NULL, "media_service_files/GetVideoSources.xml", 4,
            "%WIDTH%", stmp_w,
            "%HEIGHT%", stmp_h);

    fprintf(stdout, "Content-type: application/soap+xml\r\n");
    fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

    return cat("stdout", "media_service_files/GetVideoSources.xml", 4,
            "%WIDTH%", stmp_w,
            "%HEIGHT%", stmp_h);
}

int media_get_video_source_configurations()
{
    char profiles_num[2], stmp_w[16], stmp_h[16];

    sprintf(profiles_num, "%d", service_ctx.profiles_num);
    sprintf(stmp_w, "%d", service_ctx.profiles[0].width);
    sprintf(stmp_h, "%d", service_ctx.profiles[0].height);
    long size = cat(NULL, "media_service_files/GetVideoSourceConfigurations.xml", 6,
            "%PROFILES_NUM%", profiles_num,
            "%WIDTH%", stmp_w,
            "%HEIGHT%", stmp_h);

    fprintf(stdout, "Content-type: application/soap+xml\r\n");
    fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

    return cat("stdout", "media_service_files/GetVideoSourceConfigurations.xml", 6,
            "%PROFILES_NUM%", profiles_num,
            "%WIDTH%", stmp_w,
            "%HEIGHT%", stmp_h);
}

int media_get_video_source_configuration()
{
    const char *configuration_token = get_element("ConfigurationToken", "Body");
    char profiles_num[2], stmp_w[16], stmp_h[16];

    if (configuration_token == NULL) {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoConfig", "No config", "The requested configuration indicated does not exist");
        return -1;
    }

    if (strncasecmp("VideoSourceConfigToken", configuration_token, 22) == 0) {

        sprintf(profiles_num, "%d", service_ctx.profiles_num);
        sprintf(stmp_w, "%d", service_ctx.profiles[0].width);
        sprintf(stmp_h, "%d", service_ctx.profiles[0].height);
        long size = cat(NULL, "media_service_files/GetVideoSourceConfiguration.xml", 6,
                "%PROFILES_NUM%", profiles_num,
                "%WIDTH%", stmp_w,
                "%HEIGHT%", stmp_h);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetVideoSourceConfiguration.xml", 6,
                "%PROFILES_NUM%", profiles_num,
                "%WIDTH%", stmp_w,
                "%HEIGHT%", stmp_h);

    } else {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoConfig", "No config", "The requested configuration indicated does not exist");
        return -2;
    }
}

int media_get_compatible_video_source_configurations()
{
    // Get the video source configuration from the 1st profile
    // Ignore the requested token
    char profiles_num[2];
    char stmp_w[16], stmp_h[16];

    sprintf(profiles_num, "%d", service_ctx.profiles_num);
    sprintf(stmp_w, "%d", service_ctx.profiles[0].width);
    sprintf(stmp_h, "%d", service_ctx.profiles[0].height);
    long size = cat(NULL, "media_service_files/GetCompatibleVideoSourceConfigurations.xml", 8,
            "%PROFILES_NUM%", profiles_num,
            "%WIDTH%", stmp_w,
            "%HEIGHT%", stmp_h,
            "%PROFILE_TYPE", "High");

    fprintf(stdout, "Content-type: application/soap+xml\r\n");
    fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

    return cat("stdout", "media_service_files/GetCompatibleVideoSourceConfigurations.xml", 8,
            "%PROFILES_NUM%", profiles_num,
            "%WIDTH%", stmp_w,
            "%HEIGHT%", stmp_h,
            "%PROFILE_TYPE", "High");
}

int media_get_video_source_configuration_options()
{
    const char *configuration_token = get_element("ConfigurationToken", "Body");
    const char *profile_token = get_element("ProfileToken", "Body");
    char token[23];
    char stmp_w[16], stmp_h[16];

    memset(token, '\0', sizeof(token));
    if (profile_token != NULL) {
        // Extract "Profile_x" from token Profile_x
        strncpy(token, profile_token, 9);
    } else if (configuration_token != NULL) {
        strncpy(token, configuration_token, 22);
    } else {
        strncpy(token, service_ctx.profiles[0].name, 9);
    }

    if ((strcasecmp(service_ctx.profiles[0].name, token) == 0) ||
            (strcasecmp(service_ctx.profiles[1].name, token) == 0) ||
            (strcasecmp("VideoSourceConfigToken", token) == 0)) {

        sprintf(stmp_w, "%d", service_ctx.profiles[0].width);
        sprintf(stmp_h, "%d", service_ctx.profiles[0].height);
        long size = cat(NULL, "media_service_files/GetVideoSourceConfigurationOptions.xml", 4,
                "%WIDTH%", stmp_w,
                "%HEIGHT%", stmp_h);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetVideoSourceConfigurationOptions.xml", 4,
                "%WIDTH%", stmp_w,
                "%HEIGHT%", stmp_h);

    } else {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoConfig", "No config", "The requested configuration indicated does not exist");
        return -2;
    }
}

int media_get_profiles()
{
    char profiles_num[2];
    char stmp_w_h[16], stmp_h_h[16];
    char stmp_w_l[16], stmp_h_l[16];
    char audio_enc_h[16], audio_enc_l[16];

    audio_enc_h[0] = '\0';
    audio_enc_l[0] = '\0';
    sprintf(profiles_num, "%d", service_ctx.profiles_num);

    if (service_ctx.profiles_num == 1) {
        sprintf(stmp_w_h, "%d", service_ctx.profiles[0].width);
        sprintf(stmp_h_h, "%d", service_ctx.profiles[0].height);
        set_audio_codec(audio_enc_h, 16, service_ctx.profiles[0].audio_encoder, 1);
        long size = cat(NULL, "media_service_files/GetProfiles_high.xml", 12,
                "%PROFILES_NUM%", profiles_num,
                "%VSC_WIDTH%", stmp_w_h,
                "%VSC_HEIGHT%", stmp_h_h,
                "%VEC_WIDTH_HIGH%", stmp_w_h,
                "%VEC_HEIGHT_HIGH%", stmp_h_h,
                "%AUDIO_ENCODING_HIGH%", audio_enc_h);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetProfiles_high.xml", 12,
                "%PROFILES_NUM%", profiles_num,
                "%VSC_WIDTH%", stmp_w_h,
                "%VSC_HEIGHT%", stmp_h_h,
                "%VEC_WIDTH_HIGH%", stmp_w_h,
                "%VEC_HEIGHT_HIGH%", stmp_h_h,
                "%AUDIO_ENCODING_HIGH%", audio_enc_h);

    } else if (service_ctx.profiles_num == 2) {
        sprintf(stmp_w_h, "%d", service_ctx.profiles[0].width);
        sprintf(stmp_h_h, "%d", service_ctx.profiles[0].height);
        sprintf(stmp_w_l, "%d", service_ctx.profiles[1].width);
        sprintf(stmp_h_l, "%d", service_ctx.profiles[1].height);
        set_audio_codec(audio_enc_h, 16, service_ctx.profiles[0].audio_encoder, 1);
        set_audio_codec(audio_enc_l, 16, service_ctx.profiles[1].audio_encoder, 1);
        long size = cat(NULL, "media_service_files/GetProfiles_both.xml", 18,
                    "%PROFILES_NUM%", profiles_num,
                    "%VSC_WIDTH%", stmp_w_h,
                    "%VSC_HEIGHT%", stmp_h_h,
                    "%VEC_WIDTH_HIGH%", stmp_w_h,
                    "%VEC_HEIGHT_HIGH%", stmp_h_h,
                    "%VEC_WIDTH_LOW%", stmp_w_l,
                    "%VEC_HEIGHT_LOW%", stmp_h_l,
                    "%AUDIO_ENCODING_HIGH%", audio_enc_h,
                    "%AUDIO_ENCODING_LOW%", audio_enc_l);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetProfiles_both.xml", 18,
                    "%PROFILES_NUM%", profiles_num,
                    "%VSC_WIDTH%", stmp_w_h,
                    "%VSC_HEIGHT%", stmp_h_h,
                    "%VEC_WIDTH_HIGH%", stmp_w_h,
                    "%VEC_HEIGHT_HIGH%", stmp_h_h,
                    "%VEC_WIDTH_LOW%", stmp_w_l,
                    "%VEC_HEIGHT_LOW%", stmp_h_l,
                    "%AUDIO_ENCODING_HIGH%", audio_enc_h,
                    "%AUDIO_ENCODING_LOW%", audio_enc_l);
    } else {
        long size = cat(NULL, "media_service_files/GetProfiles_none.xml", 0);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetProfiles_none.xml", 0);
    }
}

int media_get_profile()
{
    char profiles_num[2];
    char stmp_vsc_w[16], stmp_vsc_h[16];
    char stmp_w[16], stmp_h[16];
    const char *profile_token = get_element("ProfileToken", "Body");
    char audio_enc_h[16], audio_enc_l[16];

    if (profile_token == NULL) {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoProfile", "No profile", "The requested profile token does not exist");
        return -1;
    }

    sprintf(profiles_num, "%d", service_ctx.profiles_num);

    if ((service_ctx.profiles_num > 0) &&
            (strcasecmp(service_ctx.profiles[0].name, profile_token) == 0)) {

        // Get the video source configuration from the 1st profile
        sprintf(stmp_vsc_w, "%d", service_ctx.profiles[0].width);
        sprintf(stmp_vsc_h, "%d", service_ctx.profiles[0].height);
        sprintf(stmp_w, "%d", service_ctx.profiles[0].width);
        sprintf(stmp_h, "%d", service_ctx.profiles[0].height);
        set_audio_codec(audio_enc_h, 16, service_ctx.profiles[0].audio_encoder, 1);
        long size = cat(NULL, "media_service_files/GetProfile_high.xml", 12,
                "%PROFILES_NUM%", profiles_num,
                "%VSC_WIDTH%", stmp_vsc_w,
                "%VSC_HEIGHT%", stmp_vsc_h,
                "%WIDTH%", stmp_w,
                "%HEIGHT%", stmp_h,
                "%AUDIO_ENCODING%", audio_enc_h);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetProfile_high.xml", 12,
                "%PROFILES_NUM%", profiles_num,
                "%VSC_WIDTH%", stmp_vsc_w,
                "%VSC_HEIGHT%", stmp_vsc_h,
                "%WIDTH%", stmp_w,
                "%HEIGHT%", stmp_h,
                "%AUDIO_ENCODING%", audio_enc_h);

    } else if ((service_ctx.profiles_num == 2) &&
            (strcasecmp(service_ctx.profiles[1].name, profile_token) == 0)) {

        // Get the video source configuration from the 1st profile
        sprintf(stmp_vsc_w, "%d", service_ctx.profiles[0].width);
        sprintf(stmp_vsc_h, "%d", service_ctx.profiles[0].height);
        sprintf(stmp_w, "%d", service_ctx.profiles[1].width);
        sprintf(stmp_h, "%d", service_ctx.profiles[1].height);
        set_audio_codec(audio_enc_l, 16, service_ctx.profiles[1].audio_encoder, 1);
        long size = cat(NULL, "media_service_files/GetProfile_low.xml", 12,
                "%PROFILES_NUM%", profiles_num,
                "%VSC_WIDTH%", stmp_vsc_w,
                "%VSC_HEIGHT%", stmp_vsc_h,
                "%WIDTH%", stmp_w,
                "%HEIGHT%", stmp_h,
                "%AUDIO_ENCODING%", audio_enc_l);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetProfile_low.xml", 12,
                "%PROFILES_NUM%", profiles_num,
                "%VSC_WIDTH%", stmp_vsc_w,
                "%VSC_HEIGHT%", stmp_vsc_h,
                "%WIDTH%", stmp_w,
                "%HEIGHT%", stmp_h,
                "%AUDIO_ENCODING%", audio_enc_l);
    } else {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoProfile", "No profile", "The requested profile token does not exist");
        return -2;
    }
}

int media_create_profile()
{

    if (service_ctx.adv_fault_if_unknown == 1)
        send_action_failed_fault("media_service", -1);
    else
        send_fault("media_service", "Receiver", "ter:Action", "ter:MaxNVTProfiles", "Max profile number reached", "The maximum number of supported profiles supported by the device has been reached");

    return -1;
}

int media_get_video_encoder_configurations()
{
    char stmp_w_l[16], stmp_h_l[16];
    char stmp_w_h[16], stmp_h_h[16];

    if (service_ctx.profiles_num == 1) {
        sprintf(stmp_w_h, "%d", service_ctx.profiles[0].width);
        sprintf(stmp_h_h, "%d", service_ctx.profiles[0].height);
        long size = cat(NULL, "media_service_files/GetVideoEncoderConfigurations_high.xml", 4,
                "%WIDTH_HIGH%", stmp_w_h,
                "%HEIGHT_HIGH%", stmp_h_h);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetVideoEncoderConfigurations_high.xml", 4,
                "%WIDTH_HIGH%", stmp_w_h,
                "%HEIGHT_HIGH%", stmp_h_h);

    } else if (service_ctx.profiles_num == 2) {
        sprintf(stmp_w_h, "%d", service_ctx.profiles[0].width);
        sprintf(stmp_h_h, "%d", service_ctx.profiles[0].height);
        sprintf(stmp_w_l, "%d", service_ctx.profiles[1].width);
        sprintf(stmp_h_l, "%d", service_ctx.profiles[1].height);
        long size = cat(NULL, "media_service_files/GetVideoEncoderConfigurations_both.xml", 8,
                    "%WIDTH_HIGH%", stmp_w_h,
                    "%HEIGHT_HIGH%", stmp_h_h,
                    "%WIDTH_LOW%", stmp_w_l,
                    "%HEIGHT_LOW%", stmp_h_l);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetVideoEncoderConfigurations_both.xml", 8,
                    "%WIDTH_HIGH%", stmp_w_h,
                    "%HEIGHT_HIGH%", stmp_h_h,
                    "%WIDTH_LOW%", stmp_w_l,
                    "%HEIGHT_LOW%", stmp_h_l);
    }
}

int media_get_video_encoder_configuration()
{
    char stmp_w_l[16], stmp_h_l[16];
    char stmp_w_h[16], stmp_h_h[16];
    const char *configuration_token = get_element("ConfigurationToken", "Body");
    char token[10];

    if (configuration_token == NULL) {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoConfig", "No config", "The requested configuration indicated does not exist");
        return -1;
    }

    memset(token, '\0', sizeof(token));
    strncpy(token, configuration_token, 9);

    if (strcasecmp(service_ctx.profiles[0].name, token) == 0) {

        sprintf(stmp_w_h, "%d", service_ctx.profiles[0].width);
        sprintf(stmp_h_h, "%d", service_ctx.profiles[0].height);
        long size = cat(NULL, "media_service_files/GetVideoEncoderConfiguration.xml", 8,
                "%PROFILE%", token,
                "%WIDTH%", stmp_w_h,
                "%HEIGHT%", stmp_h_h,
                "%PROFILE_TYPE%", "High");

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetVideoEncoderConfiguration.xml", 8,
                "%PROFILE%", token,
                "%WIDTH%", stmp_w_h,
                "%HEIGHT%", stmp_h_h,
                "%PROFILE_TYPE%", "High");

    } else if ((service_ctx.profiles_num == 2) &&
            (strcasecmp(service_ctx.profiles[1].name, token) == 0)) {

        sprintf(stmp_w_l, "%d", service_ctx.profiles[1].width);
        sprintf(stmp_h_l, "%d", service_ctx.profiles[1].height);
        long size = cat(NULL, "media_service_files/GetVideoEncoderConfiguration.xml", 8,
                "%PROFILE%", token,
                "%WIDTH%", stmp_w_l,
                "%HEIGHT%", stmp_h_l,
                "%PROFILE_TYPE%", "Main");

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetVideoEncoderConfiguration.xml", 8,
                "%PROFILE%", token,
                "%WIDTH%", stmp_w_l,
                "%HEIGHT%", stmp_h_l,
                "%PROFILE_TYPE%", "Main");
    } else {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoConfig", "No config", "The requested configuration indicated does not exist");
        return -2;
    }
}

int media_get_compatible_video_encoder_configurations()
{
    char stmp_w_l[16], stmp_h_l[16];
    char stmp_w_h[16], stmp_h_h[16];
    const char *profile_token = get_element("ProfileToken", "Body");

    if (profile_token == NULL) {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoProfile", "No profile", "The requested profile does not exist");
        return -1;
    }

    if (strcasecmp(service_ctx.profiles[0].name, profile_token) == 0) {

        sprintf(stmp_w_h, "%d", service_ctx.profiles[0].width);
        sprintf(stmp_h_h, "%d", service_ctx.profiles[0].height);
        long size = cat(NULL, "media_service_files/GetCompatibleVideoEncoderConfigurations.xml", 8,
                "%PROFILE%", profile_token,
                "%WIDTH%", stmp_w_h,
                "%HEIGHT%", stmp_h_h,
                "%PROFILE_TYPE%", "High");

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetCompatibleVideoEncoderConfigurations.xml", 8,
                "%PROFILE%", profile_token,
                "%WIDTH%", stmp_w_h,
                "%HEIGHT%", stmp_h_h,
                "%PROFILE_TYPE%", "High");

    } else if ((service_ctx.profiles_num == 2) &&
            (strcasecmp(service_ctx.profiles[1].name, profile_token) == 0)) {

        sprintf(stmp_w_l, "%d", service_ctx.profiles[1].width);
        sprintf(stmp_h_l, "%d", service_ctx.profiles[1].height);
        long size = cat(NULL, "media_service_files/GetCompatibleVideoEncoderConfigurations.xml", 8,
                "%PROFILE%", profile_token,
                "%WIDTH%", stmp_w_l,
                "%HEIGHT%", stmp_h_l,
                "%PROFILE_TYPE%", "Main");

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetCompatibleVideoEncoderConfigurations.xml", 8,
                "%PROFILE%", profile_token,
                "%WIDTH%", stmp_w_l,
                "%HEIGHT%", stmp_h_l,
                "%PROFILE_TYPE%", "Main");
    } else {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoProfile", "No profile", "The requested profile does not exist");
        return -2;
    }
}

int media_get_video_encoder_configuration_options()
{
    char stmp_w[16], stmp_h[16];
    const char *configuration_token = get_element("ConfigurationToken", "Body");
    const char *profile_token = get_element("ProfileToken", "Body");
    char token[10];

    memset(token, '\0', sizeof(token));
    if (configuration_token != NULL) {
        // Extract "Profile_x" from token Profile_x_VideoEncoderConfig
        strncpy(token, configuration_token, 9);
    } else if (profile_token != NULL) {
        // Extract "Profile_x" from token Profile_x
        strncpy(token, profile_token, 9);
    } else {
        strncpy(token, service_ctx.profiles[0].name, 9);
    }

    if ((service_ctx.profiles_num > 0) &&
            (strcasecmp(service_ctx.profiles[0].name, token) == 0)) {

        sprintf(stmp_w, "%d", service_ctx.profiles[0].width);
        sprintf(stmp_h, "%d", service_ctx.profiles[0].height);
        long size = cat(NULL, "media_service_files/GetVideoEncoderConfigurationOptions.xml", 6,
                "%WIDTH%", stmp_w,
                "%HEIGHT%", stmp_h,
                "%PROFILE%", "High");

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetVideoEncoderConfigurationOptions.xml", 6,
                "%WIDTH%", stmp_w,
                "%HEIGHT%", stmp_h,
                "%PROFILE%", "High");

    } else if ((service_ctx.profiles_num == 2) &&
            (strcasecmp(service_ctx.profiles[1].name, token) == 0)) {

        sprintf(stmp_w, "%d", service_ctx.profiles[1].width);
        sprintf(stmp_h, "%d", service_ctx.profiles[1].height);
        long size = cat(NULL, "media_service_files/GetVideoEncoderConfigurationOptions.xml", 6,
                "%WIDTH%", stmp_w,
                "%HEIGHT%", stmp_h,
                "%PROFILE%", "Main");

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetVideoEncoderConfigurationOptions.xml", 6,
                "%WIDTH%", stmp_w,
                "%HEIGHT%", stmp_h,
                "%PROFILE%", "Main");

    } else {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoProfile", "No profile", "The requested profile does not exist");
        return -2;
    }
}

int media_get_guaranteed_number_of_video_encoder_instances()
{
    char stmp[8];
    const char *configuration_token = get_element("ConfigurationToken", "Body");

    if ((service_ctx.profiles_num >= 0) && (service_ctx.profiles_num <= 2)) {
        sprintf(stmp, "%d", service_ctx.profiles_num);
    } else {
        send_action_failed_fault("media_service", -1);
        return -1;
    }

    if (strncasecmp("VideoSourceConfigToken", configuration_token, 22) == 0) {

        long size = cat(NULL, "media_service_files/GetGuaranteedNumberOfVideoEncoderInstances.xml", 4,
                "%TOTAL_NUMBER%", stmp,
                "%NUMBER_H264%", stmp);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetGuaranteedNumberOfVideoEncoderInstances.xml", 4,
                "%TOTAL_NUMBER%", stmp,
                "%NUMBER_H264%", stmp);

    } else {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoConfig", "No config", "The requested configuration indicated does not exist");
        return -1;
    }
}

int media_get_snapshot_uri()
{
    char address[16];
    char netmask[16];
    char *s;
    char line[MAX_LEN];
    const char *profile_token = get_element("ProfileToken", "Body");

    memset(line, '\0', sizeof(line));

    get_ip_address(address, netmask, service_ctx.ifs);

    if (profile_token == NULL) {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoProfile", "No profile", "The requested profile does not exist");
        return -1;
    }

    if (strcasecmp(service_ctx.profiles[0].name, profile_token) == 0) {

        if (service_ctx.profiles[0].snapurl == NULL) {
            send_fault("media_service", "Receiver", "ter:Action", "ter:IncompleteConfiguration", "Incomplete configuration", "The specified media profile does not contain either a reference to a video encoder configuration or a reference to a video source configuration");
            return -2;
        }

        if (sprintf(line, service_ctx.profiles[0].snapurl, address) < 0) {
            strcpy(line, service_ctx.profiles[0].snapurl);
        }
        // Escape html chars
        html_escape(line, MAX_LEN);

        long size = cat(NULL, "media_service_files/GetSnapshotUri.xml", 2,
                    "%URI%", line);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetSnapshotUri.xml", 2,
                    "%URI%", line);

    } else if ((service_ctx.profiles_num == 2) &&
            (strcasecmp(service_ctx.profiles[1].name, profile_token) == 0)) {

        if (service_ctx.profiles[1].snapurl == NULL) {
            send_fault("media_service", "Receiver", "ter:Action", "ter:IncompleteConfiguration", "Incomplete configuration", "The specified media profile does not contain either a reference to a video encoder configuration or a reference to a video source configuration");
            return -2;
        }

        if (sprintf(line, service_ctx.profiles[1].snapurl, address) < 0) {
            strcpy(line, service_ctx.profiles[1].snapurl);
        }
        // Escape html chars
        html_escape(line, MAX_LEN);

        long size = cat(NULL, "media_service_files/GetSnapshotUri.xml", 2,
                    "%URI%", line);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetSnapshotUri.xml", 2,
                    "%URI%", line);

    } else {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoProfile", "No profile", "The requested profile does not exist");
        return -3;
    }
}

int media_get_stream_uri()
{
    char address[16];
    char netmask[16];
    char *s;
    char line[MAX_LEN];
    const char *profile_token = get_element("ProfileToken", "Body");

    memset(line, '\0', sizeof(line));

    get_ip_address(address, netmask, service_ctx.ifs);

    if (profile_token == NULL) {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoProfile", "No profile", "The requested profile does not exist");
        return -1;
    }

    if (strcasecmp(service_ctx.profiles[0].name, profile_token) == 0) {

        if (service_ctx.profiles[0].url == NULL) {
            send_fault("media_service", "Receiver", "ter:Action", "ter:IncompleteConfiguration", "Incomplete configuration", "The specified media profile does contain either unused sources or encoder configurations without a corresponding source");
            return -2;
        }

        if (sprintf(line, service_ctx.profiles[0].url, address) < 0) {
            strcpy(line, service_ctx.profiles[0].url);
        }
        // Escape html chars
        html_escape(line, MAX_LEN);

        long size = cat(NULL, "media_service_files/GetStreamUri.xml", 2,
                    "%URI%", line);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetStreamUri.xml", 2,
                    "%URI%", line);

    } else if ((service_ctx.profiles_num == 2) &&
            (strcasecmp(service_ctx.profiles[1].name, profile_token) == 0)) {

        if (service_ctx.profiles[1].url == NULL) {
            send_fault("media_service", "Receiver", "ter:Action", "ter:IncompleteConfiguration", "Incomplete configuration", "The specified media profile does contain either unused sources or encoder configurations without a corresponding source");
            return -2;
        }

        if (sprintf(line, service_ctx.profiles[1].url, address) < 0) {
            strcpy(line, service_ctx.profiles[1].url);
        }
        // Escape html chars
        html_escape(line, MAX_LEN);

        long size = cat(NULL, "media_service_files/GetStreamUri.xml", 2,
                    "%URI%", line);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetStreamUri.xml", 2,
                    "%URI%", line);

    } else {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoProfile", "No profile", "The requested profile does not exist");
        return -3;
    }
}

int media_get_audio_sources()
{
    long size = cat(NULL, "media_service_files/GetAudioSources.xml", 0);

    fprintf(stdout, "Content-type: application/soap+xml\r\n");
    fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

    return cat("stdout", "media_service_files/GetAudioSources.xml", 0);
}

int media_get_audio_source_configurations()
{
    char profiles_num[2];

    sprintf(profiles_num, "%d", service_ctx.profiles_num);

    long size = cat(NULL, "media_service_files/GetAudioSourceConfigurations.xml", 2,
            "%PROFILES_NUM%", profiles_num);

    fprintf(stdout, "Content-type: application/soap+xml\r\n");
    fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

    return cat("stdout", "media_service_files/GetAudioSourceConfigurations.xml", 2,
            "%PROFILES_NUM%", profiles_num);
}

int media_get_audio_source_configuration()
{
    const char *configuration_token = get_element("ConfigurationToken", "Body");
    char profiles_num[2];

    if (configuration_token == NULL) {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoConfig", "No config", "The requested configuration indicated does not exist");
        return -1;
    }

    if (strncasecmp("AudioSourceConfigToken", configuration_token, 22) == 0) {

        sprintf(profiles_num, "%d", service_ctx.profiles_num);
        long size = cat(NULL, "media_service_files/GetAudioSourceConfiguration.xml", 2,
                "%PROFILES_NUM%", profiles_num);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetAudioSourceConfiguration.xml", 2,
                "%PROFILES_NUM%", profiles_num);

    } else {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoConfig", "No config", "The requested configuration indicated does not exist");
        return -2;
    }
}

int media_get_audio_source_configuration_options()
{
    const char *configuration_token = get_element("ConfigurationToken", "Body");
    const char *profile_token = get_element("ProfileToken", "Body");
    char token[23];

    memset(token, '\0', sizeof(token));
    if (profile_token != NULL) {
        // Extract "Profile_x" from token Profile_x
        strncpy(token, profile_token, 9);
    } else if (configuration_token != NULL) {
        strncpy(token, configuration_token, 22);
    } else {
        strncpy(token, service_ctx.profiles[0].name, 9);
    }

    if ((strcasecmp(service_ctx.profiles[0].name, token) == 0) ||
            (strcasecmp(service_ctx.profiles[1].name, token) == 0) ||
            (strcasecmp("AudioSourceConfigToken", token) == 0)) {

        long size = cat(NULL, "media_service_files/GetAudioSourceConfigurationOptions.xml", 0);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetAudioSourceConfigurationOptions.xml", 0);

    } else {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoConfig", "No config", "The requested configuration indicated does not exist");
        return -2;
    }
}

int media_get_audio_encoder_configuration()
{
    char audio_encoder[16];
    const char *configuration_token = get_element("ConfigurationToken", "Body");
    char token[10];

    if (configuration_token == NULL) {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoConfig", "No config", "The requested configuration indicated does not exist");
        return -1;
    }

    memset(token, '\0', sizeof(token));
    strncpy(token, configuration_token, 9);

    if (strcasecmp(service_ctx.profiles[0].name, token) == 0) {

        set_audio_codec(audio_encoder, 16, service_ctx.profiles[0].audio_encoder, 1);

    } else if ((service_ctx.profiles_num == 2) &&
            (strcasecmp(service_ctx.profiles[1].name, token) == 0)) {

        set_audio_codec(audio_encoder, 16, service_ctx.profiles[1].audio_encoder, 1);

    } else {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoConfig", "No config", "The requested configuration indicated does not exist");
        return -2;
    }

    long size = cat(NULL, "media_service_files/GetAudioEncoderConfiguration.xml", 4,
            "%PROFILE%", token,
            "%AUDIO_ENCODING%", audio_encoder);

    fprintf(stdout, "Content-type: application/soap+xml\r\n");
    fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

    return cat("stdout", "media_service_files/GetAudioEncoderConfiguration.xml", 4,
            "%PROFILE%", token,
            "%AUDIO_ENCODING%", audio_encoder);
}

int media_get_audio_encoder_configurations()
{
    char audio_encoder_high[16];
    char audio_encoder_low[16];

    if (service_ctx.profiles_num == 1) {
        set_audio_codec(audio_encoder_high, 16, service_ctx.profiles[0].audio_encoder, 1);
        long size = cat(NULL, "media_service_files/GetAudioEncoderConfigurations_high.xml", 2,
                "%AUDIO_ENCODING_HIGH%", audio_encoder_high);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetAudioEncoderConfigurations_high.xml", 2,
                "%AUDIO_ENCODING_HIGH%", audio_encoder_high);

    } else if (service_ctx.profiles_num == 2) {
        set_audio_codec(audio_encoder_high, 16, service_ctx.profiles[0].audio_encoder, 1);
        set_audio_codec(audio_encoder_low, 16, service_ctx.profiles[1].audio_encoder, 1);

        long size = cat(NULL, "media_service_files/GetAudioEncoderConfigurations_both.xml", 4,
                "%AUDIO_ENCODING_HIGH%", audio_encoder_high,
                "%AUDIO_ENCODING_LOW%", audio_encoder_low);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetAudioEncoderConfigurations_both.xml", 4,
                "%AUDIO_ENCODING_HIGH%", audio_encoder_high,
                "%AUDIO_ENCODING_LOW%", audio_encoder_low);
    }
}

int media_get_audio_encoder_configuration_options()
{
    char audio_encoder[16];
    const char *configuration_token = get_element("ConfigurationToken", "Body");
    const char *profile_token = get_element("ProfileToken", "Body");
    char token[10];

    memset(token, '\0', sizeof(token));
    if (configuration_token != NULL) {
        // Extract "Profile_x" from token Profile_x_VideoEncoderConfig
        strncpy(token, configuration_token, 9);
    } else if (profile_token != NULL) {
        // Extract "Profile_x" from token Profile_x
        strncpy(token, profile_token, 9);
    } else {
        strncpy(token, service_ctx.profiles[0].name, 9);
    }

    if (strcasecmp(service_ctx.profiles[0].name, token) == 0) {

        set_audio_codec(audio_encoder, 16, service_ctx.profiles[0].audio_encoder, 1);

    } else if ((service_ctx.profiles_num == 2) &&
            (strcasecmp(service_ctx.profiles[1].name, token) == 0)) {

        set_audio_codec(audio_encoder, 16, service_ctx.profiles[1].audio_encoder, 1);

    } else {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoProfile", "No profile", "The requested profile does not exist");
        return -2;
    }

    long size = cat(NULL, "media_service_files/GetAudioEncoderConfigurationOptions.xml", 2,
            "%AUDIO_ENCODING%", audio_encoder);

    fprintf(stdout, "Content-type: application/soap+xml\r\n");
    fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

    return cat("stdout", "media_service_files/GetAudioEncoderConfigurationOptions.xml", 2,
            "%AUDIO_ENCODING%", audio_encoder);
}

int media_get_audio_decoder_configuration()
{
    const char *configuration_token = get_element("ConfigurationToken", "Body");
    char token[10];

    if (configuration_token == NULL) {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoConfig", "No config", "The requested configuration indicated does not exist");
        return -1;
    }

    memset(token, '\0', sizeof(token));
    strncpy(token, configuration_token, 9);

    if (strcasecmp(service_ctx.profiles[0].name, token) == 0) {

        long size = cat(NULL, "media_service_files/GetAudioDecoderConfiguration.xml", 2,
                "%PROFILE%", token);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetAudioDecoderConfiguration.xml", 2,
                "%PROFILE%", token);

    } else if ((service_ctx.profiles_num == 2) &&
            (strcasecmp(service_ctx.profiles[1].name, token) == 0)) {

        long size = cat(NULL, "media_service_files/GetAudioDecoderConfiguration.xml", 2,
                "%PROFILE%", token);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetAudioDecoderConfiguration.xml", 2,
                "%PROFILE%", token);

    } else {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoConfig", "No config", "The requested configuration indicated does not exist");
        return -2;
    }
}

int media_get_audio_decoder_configurations()
{
    if (service_ctx.profiles_num == 1) {
        long size = cat(NULL, "media_service_files/GetAudioDecoderConfigurations_high.xml", 0);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetAudioDecoderConfigurations_high.xml", 0);

    } else if (service_ctx.profiles_num == 2) {
        long size = cat(NULL, "media_service_files/GetAudioDecoderConfigurations_both.xml", 0);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetAudioDecoderConfigurations_both.xml", 0);
    }
}

int media_get_audio_decoder_configuration_options()
{
    int decoder_type;
    const char *configuration_token = get_element("ConfigurationToken", "Body");
    const char *profile_token = get_element("ProfileToken", "Body");
    char token[10];
    char audio_decoder[16];

    memset(token, '\0', sizeof(token));
    if (configuration_token != NULL) {
        // Extract "Profile_x" from token Profile_x_VideoEncoderConfig
        strncpy(token, configuration_token, 9);
    } else if (profile_token != NULL) {
        // Extract "Profile_x" from token Profile_x
        strncpy(token, profile_token, 9);
    } else {
        strncpy(token, service_ctx.profiles[0].name, 9);
    }

    if (strcasecmp(service_ctx.profiles[0].name, token) == 0) {
        decoder_type = service_ctx.profiles[0].audio_decoder;
    } else if ((service_ctx.profiles_num == 2) && (strcasecmp(service_ctx.profiles[1].name, token) == 0)) {
        decoder_type = service_ctx.profiles[1].audio_decoder;
    }

    if (decoder_type != AUDIO_NONE) {

        set_audio_codec(audio_decoder, 16, decoder_type, 1);

        long size = cat(NULL, "media_service_files/GetAudioDecoderConfigurationOptions.xml", 2,
                "%AUDIO_DECODING%", audio_decoder);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetAudioDecoderConfigurationOptions.xml", 2,
                "%AUDIO_DECODING%", audio_decoder);

    } else {

        long size = cat(NULL, "media_service_files/GetAudioDecoderConfigurationOptions_none.xml", 0);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetAudioDecoderConfigurationOptions_none.xml", 0);
    }
}

int media_get_audio_outputs()
{
    long size = cat(NULL, "media_service_files/GetAudioOutputs.xml", 0);

    fprintf(stdout, "Content-type: application/soap+xml\r\n");
    fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

    return cat("stdout", "media_service_files/GetAudioOutputs.xml", 0);
}

int media_get_audio_output_configuration()
{
    char profiles_num[2];

    sprintf(profiles_num, "%d", service_ctx.profiles_num);

    long size = cat(NULL, "media_service_files/GetAudioOutputConfiguration.xml", 2,
            "%PROFILES_NUM%", profiles_num);

    fprintf(stdout, "Content-type: application/soap+xml\r\n");
    fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

    return cat("stdout", "media_service_files/GetAudioOutputConfiguration.xml", 2,
            "%PROFILES_NUM%", profiles_num);
}

int media_get_audio_output_configurations()
{
    char profiles_num[2];

    sprintf(profiles_num, "%d", service_ctx.profiles_num);

    long size = cat(NULL, "media_service_files/GetAudioOutputConfigurations.xml", 2,
            "%PROFILES_NUM%", profiles_num);

    fprintf(stdout, "Content-type: application/soap+xml\r\n");
    fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

    return cat("stdout", "media_service_files/GetAudioOutputConfigurations.xml", 2,
            "%PROFILES_NUM%", profiles_num);
}

int media_get_audio_output_configuration_options()
{
    long size = cat(NULL, "media_service_files/GetAudioOutputConfigurationOptions.xml", 0);

    fprintf(stdout, "Content-type: application/soap+xml\r\n");
    fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

    return cat("stdout", "media_service_files/GetAudioOutputConfigurationOptions.xml", 0);
}

int media_get_compatible_audio_source_configurations()
{
    // Ignore the requested token
    char profiles_num[2];

    sprintf(profiles_num, "%d", service_ctx.profiles_num);

    long size = cat(NULL, "media_service_files/GetCompatibleAudioSourceConfigurations.xml", 2,
            "%PROFILES_NUM%", profiles_num);

    fprintf(stdout, "Content-type: application/soap+xml\r\n");
    fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

    return cat("stdout", "media_service_files/GetCompatibleAudioSourceConfigurations.xml", 2,
            "%PROFILES_NUM%", profiles_num);
}

int media_get_compatible_audio_encoder_configurations()
{
    char audio_encoder[16];
    const char *profile_token = get_element("ProfileToken", "Body");

    if (profile_token == NULL) {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoProfile", "No profile", "The requested profile does not exist");
        return -1;
    }

    if (strcasecmp(service_ctx.profiles[0].name, profile_token) == 0) {

        set_audio_codec(audio_encoder, 16, service_ctx.profiles[0].audio_encoder, 1);
        long size = cat(NULL, "media_service_files/GetCompatibleAudioEncoderConfigurations.xml", 4,
                "%PROFILE%", profile_token,
                "%AUDIO_ENCODING%", audio_encoder);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetCompatibleAudioEncoderConfigurations.xml", 4,
                "%PROFILE%", profile_token,
                "%AUDIO_ENCODING%", audio_encoder);

    } else if ((service_ctx.profiles_num == 2) &&
            (strcasecmp(service_ctx.profiles[1].name, profile_token) == 0)) {

        set_audio_codec(audio_encoder, 16, service_ctx.profiles[1].audio_encoder, 1);
        long size = cat(NULL, "media_service_files/GetCompatibleAudioEncoderConfigurations.xml", 4,
                "%PROFILE%", profile_token,
                "%AUDIO_ENCODING%", audio_encoder);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetCompatibleAudioEncoderConfigurations.xml", 4,
                "%PROFILE%", profile_token,
                "%AUDIO_ENCODING%", audio_encoder);
    }
}

int media_get_compatible_audio_decoder_configurations()
{
    char audio_encoder_high[8];
    char audio_encoder_low[8];
    const char *profile_token = get_element("ProfileToken", "Body");

    if (profile_token == NULL) {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoProfile", "No profile", "The requested profile does not exist");
        return -1;
    }

    if (strcasecmp(service_ctx.profiles[0].name, profile_token) == 0) {
        long size = cat(NULL, "media_service_files/GetCompatibleAudioDecoderConfigurations.xml", 2,
                "%PROFILE%", profile_token);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetCompatibleAudioDecoderConfigurations.xml", 2,
                "%PROFILE%", profile_token);

    } else if ((service_ctx.profiles_num == 2) &&
            (strcasecmp(service_ctx.profiles[1].name, profile_token) == 0)) {

        long size = cat(NULL, "media_service_files/GetCompatibleAudioDecoderConfigurations.xml", 2,
                "%PROFILE%", profile_token);

        fprintf(stdout, "Content-type: application/soap+xml\r\n");
        fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

        return cat("stdout", "media_service_files/GetCompatibleAudioDecoderConfigurations.xml", 2,
                "%PROFILE%", profile_token);
    }
}

int media_get_compatible_audio_output_configurations()
{
    // Ignore the requested token
    char profiles_num[2];

    sprintf(profiles_num, "%d", service_ctx.profiles_num);

    long size = cat(NULL, "media_service_files/GetCompatibleAudioOutputConfigurations.xml", 2,
            "%PROFILES_NUM%", profiles_num);

    fprintf(stdout, "Content-type: application/soap+xml\r\n");
    fprintf(stdout, "Content-Length: %ld\r\n\r\n", size);

    return cat("stdout", "media_service_files/GetCompatibleAudioOutputConfigurations.xml", 2,
            "%PROFILES_NUM%", profiles_num);
}

int media_set_video_source_configuration()
{
    const char *token = NULL;
    ezxml_t node;

    node = get_element_ptr(NULL, "Configuration", "Body");
    if (node != NULL) {
        token = get_attribute(node, "token");
    }

    if ((node == NULL) || (token == NULL)) {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoConfig", "No config", "The configuration does not exist");
        return -1;
    }

    if (strcasecmp("VideoSourceConfigToken", token) == 0) {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:ConfigModify", "Config modify", "The configuration parameters are not possible to set");
        return -2;
    } else {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoConfig", "No config", "The configuration does not exist");
        return -1;
    }
}

int media_set_audio_source_configuration()
{
    const char *token = NULL;
    ezxml_t node;

    node = get_element_ptr(NULL, "Configuration", "Body");
    if (node != NULL) {
        token = get_attribute(node, "token");
    }

    if ((node == NULL) || (token == NULL)) {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoConfig", "No config", "The configuration does not exist");
        return -1;
    }

    if (strcasecmp("AudioSourceConfigToken", token) == 0) {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:ConfigModify", "Config modify", "The configuration parameters are not possible to set");
        return -2;
    } else {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoConfig", "No config", "The configuration does not exist");
        return -1;
    }
}

int media_set_video_encoder_configuration()
{
    send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:ConfigModify", "Config modify", "The configuration parameters are not possible to set");
    return -1;
}

int media_set_audio_encoder_configuration()
{
    send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:ConfigModify", "Config modify", "The configuration parameters are not possible to set");
    return -1;
}

int media_set_audio_output_configuration()
{
    const char *token = NULL;
    ezxml_t node;

    node = get_element_ptr(NULL, "Configuration", "Body");
    if (node != NULL) {
        token = get_attribute(node, "token");
    }

    if ((node == NULL) || (token == NULL)) {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoConfig", "No config", "The configuration does not exist");
        return -1;
    }

    if (strcasecmp("AudioOutputConfigToken", token) == 0) {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:ConfigModify", "Config modify", "The configuration parameters are not possible to set");
        return -2;
    } else {
        send_fault("media_service", "Sender", "ter:InvalidArgVal", "ter:NoConfig", "No config", "The configuration does not exist");
        return -1;
    }
}

int media_unsupported(const char *method)
{
    if (service_ctx.adv_fault_if_unknown == 1)
        send_action_failed_fault("media_service", -1);
    else
        send_empty_response("trt", (char *) method);
    return -1;
}
