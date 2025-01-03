struct request {
    std::string method;
    std::string url;
    std::string httpVersion;
    std::map<std::string, std::string> headers;
    std::string body;

    size_t maxHeader = 100;
    size_t maxBuffer = 8196 * 2;
    size_t maxBody = 20000;

    inline request() : method("GET"), httpVersion("HTTP/1.1") {}

    inline void addHeader(const std::string& key, const std::string& value) {
        headers[key] = value;
    }

    inline void clear() {
        method.clear();
        url.clear();
        httpVersion.clear();
        headers.clear();
        body.clear();
        receivedBuffer.clear();
        status = recStatus::reqLine;
        bodylen = 0;
        chunk = -1;
    }

    inline std::string toString() {
        std::ostringstream requestStream;

        if (headers.find("Content-Length") == headers.end())
        {
            headers["Content-Length"] = std::to_string(body.length());
        }

        requestStream << method << " " << url << " " << httpVersion << "\r\n";

        for (const auto& header : headers) {
            requestStream << header.first << ": " << header.second << "\r\n";
        }

        requestStream << "\r\n";

        requestStream << body;

        return requestStream.str();
    }

    inline io::err fromChar(const char* rawData, size_t size) {
        if (status != recStatus::reqBody)
        {
            if (receivedBuffer.size() + size > maxBuffer)
                return io::err::bufoverf;
            receivedBuffer.append(rawData, size);
            while (true)
            {
                size_t endOfLine;
                endOfLine = receivedBuffer.find("\r\n");
                if (endOfLine == std::string::npos) {
                    return io::err::less;
                }

                switch (status)
                {
                case recStatus::reqLine:
                {
                    std::istringstream requestLine(receivedBuffer.substr(0, endOfLine));
                    requestLine >> method >> url >> httpVersion;
                    receivedBuffer = receivedBuffer.substr(endOfLine + 2);
                    status = recStatus::reqHead;
                }
                break;
                case recStatus::reqHead:
                {
                    if (endOfLine != 0)
                    {
                        if (maxHeader < headers.size())
                            return io::err::bufoverf;
                        size_t colonPos = receivedBuffer.find(':');
                        if (colonPos != std::string::npos)
                        {
                            std::string key = receivedBuffer.substr(0, colonPos);

                            std::string value = receivedBuffer.substr(colonPos + 1, endOfLine - colonPos - 1);
                            value.erase(0, value.find_first_not_of(" "));
                            value.erase(value.find_last_not_of(" ") + 1);

                            headers[key] = value;
                        }
                    }
                    else
                    {
                        status = recStatus::reqBody;
                        if (headers.find("Content-Length") != headers.end()) {
                            std::string lengthstr = headers["Content-Length"];
                            std::stringstream ss(lengthstr);
                            ss >> bodylen;
                        } else {
                            auto it = headers.find("Transfer-Encoding");
                            if (it != headers.end()) {
                                if (it->second.find("chunked") == std::string::npos) {
                                    return io::err::formaterr;
                                }
                                chunk = 0;
                                std::string restof = receivedBuffer.substr(endOfLine + 2);
                                receivedBuffer.clear();
                                return this->fromChar(restof.c_str(), restof.length());
                            }
                        }
                        //fixed size process
                        body = receivedBuffer.substr(endOfLine + 2);
                        receivedBuffer.clear();
                        if (body.length() >= bodylen)
                            return io::err::ok;
                        else
                            return io::err::less;
                    }
                    receivedBuffer = receivedBuffer.substr(endOfLine + 2);
                }
                break;
                }
            }
        }
        else if (chunk == -1)
        {
            if (body.size() + size > maxBody)
                return io::err::bufoverf;
            body.append(rawData, size);
            if (body.length() >= bodylen)
                return io::err::ok;
            else
                return io::err::less;
        }
        else
        {
            while(true)
            {
                if (chunk == 0)
                {
                    if (receivedBuffer.size() + size > maxBuffer)
                        return io::err::bufoverf;
                    receivedBuffer.append(rawData, size);

                    size_t endOfLine;
                    while(1)
                    {
                        endOfLine = receivedBuffer.find("\r\n");
                        if (endOfLine == std::string::npos) {
                            return io::err::less;
                        }
                        else if (endOfLine == 0) {
                            receivedBuffer = receivedBuffer.substr(endOfLine + 2);
                        }
                        else break;
                    }
                    std::string subString = receivedBuffer.substr(0, endOfLine);
                    receivedBuffer = receivedBuffer.substr(endOfLine + 2);
                    std::stringstream ss;
                    ss << std::hex << subString;
                    if (chunk == 0)
                        return io::err::ok;
                    if (chunk > maxBody)
                        return io::err::bufoverf;
                    if (receivedBuffer.size())
                        return this->fromChar(receivedBuffer.c_str(), receivedBuffer.size());
                }
                else
                {
                    if(size < chunk)
                    {
                        if (body.size() + size > maxBody)
                            return io::err::bufoverf;
                        body.append(rawData, size);
                        chunk -= size;
                        return io::err::less;
                    }
                    else
                    {
                        if (body.size() + chunk > maxBody)
                            return io::err::bufoverf;
                        body.append(rawData, chunk);
                        rawData += chunk;
                        size -= chunk;
                        chunk = 0;
                        receivedBuffer.clear();
                    }
                }
            }
        }
        return io::err::failed;
    }

private:
    std::string receivedBuffer;
    enum class recStatus {
        reqLine,
        reqHead,
        reqBody
    }status = recStatus::reqLine;
    size_t bodylen = 0;
    int chunk = -1;
};

struct responce {
    std::string httpVersion;
    int statusCode = 0;
    std::string reasonPhrase;
    std::map<std::string, std::string> headers;
    std::string body;

    size_t maxHeader = 100;
    size_t maxBuffer = 8196 * 2;
    size_t maxBody = 1000000;

    inline void addHeader(const std::string& key, const std::string& value) {
        headers[key] = value;
    }

    inline void clear() {
        httpVersion.clear();
        statusCode = 200;
        reasonPhrase = "OK";
        headers.clear();
        body.clear();
        status = recStatus::rspLine;
        bodylen = 0;
        chunk = -1;
    }

    inline std::string toString() {
        std::ostringstream responseStream;

        if (headers.find("Content-Length") == headers.end())
        {
            headers["Content-Length"] = std::to_string(body.length());
        }
        
        responseStream << httpVersion << " " << statusCode << " " << reasonPhrase << "\r\n";

        for (const auto& header : headers) {
            responseStream << header.first << ": " << header.second << "\r\n";
        }

        responseStream << "\r\n";

        responseStream << body;

        return responseStream.str();
    }

    inline io::err fromChar(const char* rawData, size_t size) {
        if (status != recStatus::rspBody)
        {
            if (receivedBuffer.size() + size > maxBuffer)
                return io::err::bufoverf;
            receivedBuffer.append(rawData, size);
            while (true)
            {
                size_t endOfLine;
                endOfLine = receivedBuffer.find("\r\n");
                if (endOfLine == std::string::npos) {
                    return io::err::less;
                }

                switch (status)
                {
                case recStatus::rspLine:
                {
                    std::string statusCodeString;
                    std::istringstream requestLine(receivedBuffer.substr(0, endOfLine));
                    requestLine >> httpVersion >> statusCodeString >> reasonPhrase;
                    std::stringstream ss(statusCodeString);
                    ss >> statusCode;
                    receivedBuffer = receivedBuffer.substr(endOfLine + 2);
                    status = recStatus::rspHead;
                }
                break;
                case recStatus::rspHead:
                {
                    if (endOfLine != 0)
                    {
                        if (maxHeader < headers.size())
                            return io::err::bufoverf;
                        size_t colonPos = receivedBuffer.find(':');
                        if (colonPos != std::string::npos) {
                            std::string key = receivedBuffer.substr(0, colonPos);

                            std::string value = receivedBuffer.substr(colonPos + 1, endOfLine - colonPos - 1);
                            value.erase(0, value.find_first_not_of(" "));
                            value.erase(value.find_last_not_of(" ") + 1);

                            headers[key] = value;
                        }
                    }
                    else
                    {
                        status = recStatus::rspBody;
                        if (headers.find("Content-Length") != headers.end()) {
                            std::string lengthstr = headers["Content-Length"];
                            std::stringstream ss(lengthstr);
                            ss >> bodylen;
                        } else {
                            auto it = headers.find("Transfer-Encoding");
                            if (it != headers.end()) {
                                if (it->second.find("chunked") == std::string::npos) {
                                    return io::err::formaterr;
                                }
                                chunk = 0;
                                std::string restof = receivedBuffer.substr(endOfLine + 2);
                                receivedBuffer.clear();
                                return this->fromChar(restof.c_str(), restof.length());
                            }
                        }
                        //fixed size process
                        body = receivedBuffer.substr(endOfLine + 2);
                        receivedBuffer.clear();
                        if (body.length() >= bodylen)
                            return io::err::ok;
                        else
                            return io::err::less;
                    }
                    receivedBuffer = receivedBuffer.substr(endOfLine + 2);
                }
                break;
                }
            }
        }
        else if (chunk == -1)
        {
            if (body.size() + size > maxBody)
                return io::err::bufoverf;
            body.append(rawData, size);
            if (body.length() >= bodylen)
                return io::err::ok;
            else
                return io::err::less;
        }
        else
        {
            while(true)
            {
                if (chunk == 0)
                {
                    if (receivedBuffer.size() + size > maxBuffer)
                        return io::err::bufoverf;
                    receivedBuffer.append(rawData, size);

                    size_t endOfLine;
                    while(1)
                    {
                        endOfLine = receivedBuffer.find("\r\n");
                        if (endOfLine == std::string::npos) {
                            return io::err::less;
                        }
                        else if (endOfLine == 0) {
                            receivedBuffer = receivedBuffer.substr(endOfLine + 2);
                        }
                        else break;
                    }
                    std::string subString = receivedBuffer.substr(0, endOfLine);
                    receivedBuffer = receivedBuffer.substr(endOfLine + 2);
                    std::stringstream ss;
                    ss << std::hex << subString;
                    ss >> chunk;
                    if (chunk == 0)
                        return io::err::ok;
                    if (chunk > maxBody)
                        return io::err::bufoverf;
                    if (receivedBuffer.size())
                        return this->fromChar(receivedBuffer.c_str(), receivedBuffer.size());
                }
                else
                {
                    if(size < chunk)
                    {
                        if (body.size() + size > maxBody)
                            return io::err::bufoverf;
                        body.append(rawData, size);
                        chunk -= size;
                        return io::err::less;
                    }
                    else
                    {
                        if (body.size() + chunk > maxBody)
                            return io::err::bufoverf;
                        body.append(rawData, chunk);
                        rawData += chunk;
                        size -= chunk;
                        chunk = 0;
                        receivedBuffer.clear();
                    }
                }
            }
        }
        return io::err::failed;
    }

private:
    std::string receivedBuffer;
    enum class recStatus {
        rspLine,
        rspHead,
        rspBody
    }status = recStatus::rspLine;
    size_t bodylen = 0;
    int chunk = -1;
};