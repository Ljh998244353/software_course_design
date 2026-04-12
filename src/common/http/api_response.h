#pragma once

#include <string>

#include <json/json.h>

#include "common/errors/error_code.h"

namespace auction::common::http {

struct ApiResponse {
    errors::ErrorCode code{errors::ErrorCode::kOk};
    std::string message{"success"};
    Json::Value data{Json::objectValue};

    [[nodiscard]] Json::Value ToJsonValue() const {
        Json::Value root(Json::objectValue);
        root["code"] = static_cast<int>(code);
        root["message"] = message;
        root["data"] = data;
        return root;
    }

    [[nodiscard]] std::string ToJsonString() const {
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";
        return Json::writeString(builder, ToJsonValue());
    }

    [[nodiscard]] static ApiResponse Success(Json::Value payload = Json::Value(Json::objectValue)) {
        return ApiResponse{
            .code = errors::ErrorCode::kOk,
            .message = "success",
            .data = std::move(payload),
        };
    }

    [[nodiscard]] static ApiResponse Failure(
        const errors::ErrorCode error_code,
        std::string custom_message = {},
        Json::Value payload = Json::Value(Json::objectValue)
    ) {
        if (custom_message.empty()) {
            custom_message = std::string(errors::ErrorCodeMessage(error_code));
        }
        return ApiResponse{
            .code = error_code,
            .message = std::move(custom_message),
            .data = std::move(payload),
        };
    }
};

}  // namespace auction::common::http
