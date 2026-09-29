#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <limits>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    using Socket = SOCKET;
    using SocketLength = int;
    const Socket INVALID_SOCKET_VALUE = INVALID_SOCKET;
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    using Socket = int;
    using SocketLength = socklen_t;
    const Socket INVALID_SOCKET_VALUE = -1;
#endif

using namespace std;

const string USERNAME = "admin";
const string PASSWORD = "admin123";
const string SESSION_TOKEN = "MATRIXLAB_LOGIN_2026";

using Matrix = vector<vector<double>>;

// ============================================================
// PLATFORM HELPERS
// ============================================================

bool startNetworking()
{
#ifdef _WIN32
    WSADATA wsaData{};
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#else
    return true;
#endif
}

void stopNetworking()
{
#ifdef _WIN32
    WSACleanup();
#endif
}

void closeSocket(Socket s)
{
#ifdef _WIN32
    closesocket(s);
#else
    close(s);
#endif
}

// ============================================================
// FILE
// ============================================================

string readFile(const string& filename)
{
    ifstream file(filename);

    if (!file.is_open())
        return "";

    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// ============================================================
// URL DECODER
// ============================================================

string urlDecode(const string& value)
{
    string result;

    for (size_t i = 0; i < value.length(); i++)
    {
        if (value[i] == '%' && i + 2 < value.length())
        {
            string hex = value.substr(i + 1, 2);
            char ch = static_cast<char>(
                strtol(hex.c_str(), nullptr, 16)
            );
            result += ch;
            i += 2;
        }
        else if (value[i] == '+')
        {
            result += ' ';
        }
        else
        {
            result += value[i];
        }
    }

    return result;
}

// ============================================================
// FORM VALUE
// ============================================================

string getFormValue(const string& body, const string& key)
{
    string searchKey = key + "=";
    size_t start = body.find(searchKey);

    if (start == string::npos)
        return "";

    start += searchKey.length();

    size_t end = body.find('&', start);

    if (end == string::npos)
        end = body.length();

    return urlDecode(body.substr(start, end - start));
}

// ============================================================
// LOGIN CHECK
// ============================================================

bool isLoggedIn(const string& request)
{
    return request.find(
        "matrix_session=" + SESSION_TOKEN
    ) != string::npos;
}

// ============================================================
// HTTP RESPONSE
// ============================================================

void sendResponse(
    Socket client,
    const string& status,
    const string& contentType,
    const string& body,
    const string& extraHeaders = ""
)
{
    string response =
        "HTTP/1.1 " + status + "\r\n"
        "Content-Type: " + contentType + "\r\n"
        "Content-Length: " + to_string(body.size()) + "\r\n"
        "Connection: close\r\n"
        + extraHeaders +
        "\r\n" +
        body;

    size_t sentTotal = 0;

    while (sentTotal < response.size())
    {
        size_t remaining = response.size() - sentTotal;
        int chunkSize = static_cast<int>(
            min(remaining,
                static_cast<size_t>(numeric_limits<int>::max()))
        );

        int sent = ::send(
            client,
            response.data() + sentTotal,
            chunkSize,
            0
        );

        if (sent <= 0)
            break;

        sentTotal += static_cast<size_t>(sent);
    }
}

void sendJson(
    Socket client,
    const string& status,
    const string& json
)
{
    sendResponse(
        client,
        status,
        "application/json; charset=utf-8",
        json
    );
}

// ============================================================
// MATRIX OPERATIONS
// ============================================================

Matrix addMatrix(const Matrix& A, const Matrix& B)
{
    int rows = static_cast<int>(A.size());
    int cols = static_cast<int>(A[0].size());

    Matrix result(rows, vector<double>(cols));

    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            result[i][j] = A[i][j] + B[i][j];

    return result;
}

Matrix subtractMatrix(const Matrix& A, const Matrix& B)
{
    int rows = static_cast<int>(A.size());
    int cols = static_cast<int>(A[0].size());

    Matrix result(rows, vector<double>(cols));

    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            result[i][j] = A[i][j] - B[i][j];

    return result;
}

Matrix multiplyMatrix(const Matrix& A, const Matrix& B)
{
    int rowsA = static_cast<int>(A.size());
    int colsA = static_cast<int>(A[0].size());
    int rowsB = static_cast<int>(B.size());
    int colsB = static_cast<int>(B[0].size());

    if (colsA != rowsB)
        throw runtime_error(
            "For multiplication, columns of Matrix A must equal rows of Matrix B."
        );

    Matrix result(rowsA, vector<double>(colsB, 0));

    for (int i = 0; i < rowsA; i++)
        for (int j = 0; j < colsB; j++)
            for (int k = 0; k < colsA; k++)
                result[i][j] += A[i][k] * B[k][j];

    return result;
}

Matrix transposeMatrix(const Matrix& A)
{
    int rows = static_cast<int>(A.size());
    int cols = static_cast<int>(A[0].size());

    Matrix result(cols, vector<double>(rows));

    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            result[j][i] = A[i][j];

    return result;
}

double determinant(const Matrix& A)
{
    int n = static_cast<int>(A.size());

    if (n == 1)
        return A[0][0];

    if (n == 2)
        return A[0][0] * A[1][1] -
               A[0][1] * A[1][0];

    double det = 0;

    for (int col = 0; col < n; col++)
    {
        Matrix subMatrix;

        for (int i = 1; i < n; i++)
        {
            vector<double> row;

            for (int j = 0; j < n; j++)
                if (j != col)
                    row.push_back(A[i][j]);

            subMatrix.push_back(row);
        }

        double sign = (col % 2 == 0) ? 1.0 : -1.0;

        det += sign * A[0][col] * determinant(subMatrix);
    }

    return det;
}

Matrix inverseMatrix(const Matrix& A)
{
    int n = static_cast<int>(A.size());

    double det = determinant(A);

    if (fabs(det) < 1e-10)
        throw runtime_error(
            "Matrix inverse does not exist because the determinant is zero."
        );

    Matrix augmented(n, vector<double>(2 * n));

    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
            augmented[i][j] = A[i][j];

        for (int j = 0; j < n; j++)
            augmented[i][j + n] = (i == j) ? 1.0 : 0.0;
    }

    for (int i = 0; i < n; i++)
    {
        double pivot = augmented[i][i];

        if (fabs(pivot) < 1e-10)
        {
            int swapRow = -1;

            for (int k = i + 1; k < n; k++)
            {
                if (fabs(augmented[k][i]) > 1e-10)
                {
                    swapRow = k;
                    break;
                }
            }

            if (swapRow == -1)
                throw runtime_error(
                    "Matrix inverse does not exist."
                );

            swap(augmented[i], augmented[swapRow]);
            pivot = augmented[i][i];
        }

        for (int j = 0; j < 2 * n; j++)
            augmented[i][j] /= pivot;

        for (int k = 0; k < n; k++)
        {
            if (k == i)
                continue;

            double factor = augmented[k][i];

            for (int j = 0; j < 2 * n; j++)
                augmented[k][j] -= factor * augmented[i][j];
        }
    }

    Matrix result(n, vector<double>(n));

    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            result[i][j] = augmented[i][j + n];

    return result;
}

Matrix scalarMultiply(const Matrix& A, double scalar)
{
    int rows = static_cast<int>(A.size());
    int cols = static_cast<int>(A[0].size());

    Matrix result(rows, vector<double>(cols));

    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            result[i][j] = A[i][j] * scalar;

    return result;
}

double traceMatrix(const Matrix& A)
{
    int rows = static_cast<int>(A.size());
    int cols = static_cast<int>(A[0].size());

    if (rows != cols)
        throw runtime_error(
            "Trace is defined only for a square matrix."
        );

    double result = 0;

    for (int i = 0; i < rows; i++)
        result += A[i][i];

    return result;
}

Matrix identityMatrix(int n)
{
    Matrix result(n, vector<double>(n, 0));

    for (int i = 0; i < n; i++)
        result[i][i] = 1;

    return result;
}

// ============================================================
// JSON HELPERS
// ============================================================

vector<double> extractNumbers(const string& text)
{
    vector<double> numbers;
    string current;

    for (char c : text)
    {
        bool valid =
            (c >= '0' && c <= '9') ||
            c == '-' ||
            c == '+' ||
            c == '.' ||
            c == 'e' ||
            c == 'E';

        if (valid)
        {
            current += c;
        }
        else if (!current.empty())
        {
            try
            {
                numbers.push_back(stod(current));
            }
            catch (...)
            {
            }

            current.clear();
        }
    }

    if (!current.empty())
    {
        try
        {
            numbers.push_back(stod(current));
        }
        catch (...)
        {
        }
    }

    return numbers;
}

string getJsonValue(
    const string& json,
    const string& key
)
{
    string search = "\"" + key + "\"";

    size_t pos = json.find(search);

    if (pos == string::npos)
        return "";

    pos = json.find(':', pos);

    if (pos == string::npos)
        return "";

    pos++;

    while (
        pos < json.length() &&
        (
            json[pos] == ' ' ||
            json[pos] == '\t' ||
            json[pos] == '\r' ||
            json[pos] == '\n'
        )
    )
    {
        pos++;
    }

    if (pos < json.length() && json[pos] == '"')
    {
        pos++;

        size_t end = pos;

        while (end < json.length())
        {
            if (
                json[end] == '"' &&
                (end == 0 || json[end - 1] != '\\')
            )
            {
                break;
            }

            end++;
        }

        return json.substr(pos, end - pos);
    }

    size_t end = json.find_first_of(",}", pos);

    if (end == string::npos)
        end = json.length();

    return json.substr(pos, end - pos);
}

double getJsonNumber(
    const string& json,
    const string& key,
    double defaultValue = 0
)
{
    string value = getJsonValue(json, key);

    if (value.empty())
        return defaultValue;

    try
    {
        return stod(value);
    }
    catch (...)
    {
        return defaultValue;
    }
}

Matrix parseMatrix(
    const string& json,
    const string& key,
    int rows,
    int cols
)
{
    string search = "\"" + key + "\"";

    size_t keyPos = json.find(search);

    if (keyPos == string::npos)
        throw runtime_error(
            "Matrix " + key + " not found."
        );

    size_t start = json.find('[', keyPos);

    if (start == string::npos)
        throw runtime_error(
            "Invalid matrix format."
        );

    int depth = 0;
    size_t end = start;

    for (size_t i = start; i < json.length(); i++)
    {
        if (json[i] == '[')
            depth++;
        else if (json[i] == ']')
        {
            depth--;

            if (depth == 0)
            {
                end = i;
                break;
            }
        }
    }

    if (depth != 0)
        throw runtime_error(
            "Invalid matrix brackets."
        );

    string matrixText =
        json.substr(start, end - start + 1);

    vector<double> numbers =
        extractNumbers(matrixText);

    if (
        static_cast<int>(numbers.size()) !=
        rows * cols
    )
    {
        throw runtime_error(
            "Invalid number of matrix values."
        );
    }

    Matrix matrix(rows, vector<double>(cols));

    int index = 0;

    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            matrix[i][j] = numbers[index++];

    return matrix;
}

string formatNumber(double value)
{
    if (fabs(value) < 1e-10)
        value = 0;

    ostringstream out;
    out.setf(ios::fixed);
    out.precision(4);
    out << value;

    string result = out.str();

    while (
        result.size() > 1 &&
        result.back() == '0'
    )
    {
        result.pop_back();
    }

    if (
        !result.empty() &&
        result.back() == '.'
    )
    {
        result.pop_back();
    }

    return result;
}

string matrixToText(const Matrix& matrix)
{
    string result;

    for (size_t i = 0; i < matrix.size(); i++)
    {
        result += "[ ";

        for (size_t j = 0; j < matrix[i].size(); j++)
        {
            result += formatNumber(matrix[i][j]);

            if (j + 1 < matrix[i].size())
                result += "    ";
        }

        result += " ]";

        if (i + 1 < matrix.size())
            result += "\n";
    }

    return result;
}

string escapeJsonString(const string& value)
{
    string result;

    for (char c : value)
    {
        if (c == '\\')
            result += "\\\\";
        else if (c == '"')
            result += "\\\"";
        else if (c == '\n')
            result += "\\n";
        else if (c == '\r')
            result += "\\r";
        else if (c == '\t')
            result += "\\t";
        else
            result += c;
    }

    return result;
}

// ============================================================
// CALCULATE API
// ============================================================

void handleCalculate(
    Socket client,
    const string& request
)
{
    if (!isLoggedIn(request))
    {
        sendJson(
            client,
            "401 Unauthorized",
            "{\"success\":false,\"message\":\"Please login first.\"}"
        );
        return;
    }

    size_t bodyPos = request.find("\r\n\r\n");

    if (bodyPos == string::npos)
    {
        sendJson(
            client,
            "400 Bad Request",
            "{\"success\":false,\"message\":\"Request body missing.\"}"
        );
        return;
    }

    string body = request.substr(bodyPos + 4);

    try
    {
        string operation =
            getJsonValue(body, "operation");

        int rowsA =
            static_cast<int>(
                getJsonNumber(body, "rowsA", 2)
            );

        int colsA =
            static_cast<int>(
                getJsonNumber(body, "colsA", 2)
            );

        int rowsB =
            static_cast<int>(
                getJsonNumber(body, "rowsB", 2)
            );

        int colsB =
            static_cast<int>(
                getJsonNumber(body, "colsB", 2)
            );

        double scalar =
            getJsonNumber(body, "scalar", 1);

        if (
            rowsA <= 0 || colsA <= 0 ||
            rowsA > 20 || colsA > 20
        )
        {
            throw runtime_error(
                "Matrix A dimensions must be between 1 and 20."
            );
        }

        Matrix A =
            parseMatrix(
                body,
                "matrixA",
                rowsA,
                colsA
            );

        Matrix result;

        if (operation == "addition")
        {
            if (rowsA != rowsB || colsA != colsB)
                throw runtime_error(
                    "For addition, both matrices must have the same dimensions."
                );

            Matrix B =
                parseMatrix(
                    body,
                    "matrixB",
                    rowsB,
                    colsB
                );

            result = addMatrix(A, B);
        }
        else if (operation == "subtraction")
        {
            if (rowsA != rowsB || colsA != colsB)
                throw runtime_error(
                    "For subtraction, both matrices must have the same dimensions."
                );

            Matrix B =
                parseMatrix(
                    body,
                    "matrixB",
                    rowsB,
                    colsB
                );

            result = subtractMatrix(A, B);
        }
        else if (operation == "multiplication")
        {
            Matrix B =
                parseMatrix(
                    body,
                    "matrixB",
                    rowsB,
                    colsB
                );

            result = multiplyMatrix(A, B);
        }
        else if (operation == "transpose")
        {
            result = transposeMatrix(A);
        }
        else if (operation == "inverse")
        {
            if (rowsA != colsA)
                throw runtime_error(
                    "Inverse requires a square matrix."
                );

            result = inverseMatrix(A);
        }
        else if (operation == "scalar")
        {
            result = scalarMultiply(A, scalar);
        }
        else if (operation == "identity")
        {
            if (rowsA != colsA)
                throw runtime_error(
                    "Identity matrix requires a square size."
                );

            result = identityMatrix(rowsA);
        }
        else if (operation == "determinant")
        {
            if (rowsA != colsA)
                throw runtime_error(
                    "Determinant requires a square matrix."
                );

            double det = determinant(A);

            sendJson(
                client,
                "200 OK",
                "{\"success\":true,\"type\":\"number\",\"result\":\"" +
                formatNumber(det) +
                "\"}"
            );

            return;
        }
        else if (operation == "trace")
        {
            double trace = traceMatrix(A);

            sendJson(
                client,
                "200 OK",
                "{\"success\":true,\"type\":\"number\",\"result\":\"" +
                formatNumber(trace) +
                "\"}"
            );

            return;
        }
        else
        {
            throw runtime_error(
                "Unknown matrix operation."
            );
        }

        string resultText =
            matrixToText(result);

        string escapedResult =
            escapeJsonString(resultText);

        string response =
            "{\"success\":true,"
            "\"type\":\"matrix\","
            "\"result\":\"" +
            escapedResult +
            "\"}";

        sendJson(
            client,
            "200 OK",
            response
        );
    }
    catch (const exception& e)
    {
        string message =
            escapeJsonString(e.what());

        sendJson(
            client,
            "400 Bad Request",
            "{\"success\":false,\"message\":\"" +
            message +
            "\"}"
        );
    }
}

// ============================================================
// PORT
// ============================================================

int getPort()
{
    const char* portEnv = getenv("PORT");

    if (portEnv != nullptr)
    {
        try
        {
            int port = stoi(portEnv);

            if (port > 0 && port <= 65535)
                return port;
        }
        catch (...)
        {
        }
    }

    return 8080;
}

// ============================================================
// MAIN
// ============================================================

int main()
{
    if (!startNetworking())
    {
        cerr << "Network initialization failed." << endl;
        return 1;
    }

    int port = getPort();

    Socket serverSocket =
        socket(
            AF_INET,
            SOCK_STREAM,
            0
        );

    if (serverSocket == INVALID_SOCKET_VALUE)
    {
        cerr << "Socket creation failed." << endl;
        stopNetworking();
        return 1;
    }

    int opt = 1;

#ifdef _WIN32
    setsockopt(
        serverSocket,
        SOL_SOCKET,
        SO_REUSEADDR,
        reinterpret_cast<const char*>(&opt),
        sizeof(opt)
    );
#else
    setsockopt(
        serverSocket,
        SOL_SOCKET,
        SO_REUSEADDR,
        &opt,
        sizeof(opt)
    );
#endif

    sockaddr_in serverAddress{};

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(
        static_cast<unsigned short>(port)
    );

    if (
        bind(
            serverSocket,
            reinterpret_cast<sockaddr*>(&serverAddress),
            sizeof(serverAddress)
        ) < 0
    )
    {
        cerr << "Bind failed on port " << port << "." << endl;
        closeSocket(serverSocket);
        stopNetworking();
        return 1;
    }

    if (listen(serverSocket, 20) < 0)
    {
        cerr << "Listen failed." << endl;
        closeSocket(serverSocket);
        stopNetworking();
        return 1;
    }

    cout << "=====================================" << endl;
    cout << "       MATRIXLAB C++ SERVER" << endl;
    cout << "=====================================" << endl;
    cout << "Host: 0.0.0.0" << endl;
    cout << "Port: " << port << endl;
    cout << "Login: admin / admin123" << endl;
    cout << "Server started successfully." << endl;
    cout << "=====================================" << endl;

    while (true)
    {
        sockaddr_in clientAddress{};

        SocketLength clientSize =
            static_cast<SocketLength>(
                sizeof(clientAddress)
            );

        Socket clientSocket =
            accept(
                serverSocket,
                reinterpret_cast<sockaddr*>(&clientAddress),
                &clientSize
            );

        if (clientSocket == INVALID_SOCKET_VALUE)
            continue;

        string request;
        char buffer[65536];

        while (true)
        {
            int received =
                recv(
                    clientSocket,
                    buffer,
                    sizeof(buffer),
                    0
                );

            if (received <= 0)
                break;

            request.append(buffer, received);

            size_t headerEnd =
                request.find("\r\n\r\n");

            if (headerEnd == string::npos)
                continue;

            size_t contentLengthPos =
                request.find("Content-Length:");

            if (contentLengthPos != string::npos)
            {
                size_t valueStart =
                    contentLengthPos + 15;

                while (
                    valueStart < request.size() &&
                    request[valueStart] == ' '
                )
                {
                    valueStart++;
                }

                size_t valueEnd =
                    request.find("\r\n", valueStart);

                if (valueEnd != string::npos)
                {
                    try
                    {
                        size_t contentLength =
                            stoul(
                                request.substr(
                                    valueStart,
                                    valueEnd - valueStart
                                )
                            );

                        size_t bodyStart =
                            headerEnd + 4;

                        if (
                            request.size() >=
                            bodyStart + contentLength
                        )
                        {
                            break;
                        }
                    }
                    catch (...)
                    {
                        break;
                    }
                }
            }
            else
            {
                break;
            }
        }

        if (request.empty())
        {
            closeSocket(clientSocket);
            continue;
        }

        size_t firstSpace =
            request.find(' ');

        size_t secondSpace =
            request.find(' ', firstSpace + 1);

        if (
            firstSpace == string::npos ||
            secondSpace == string::npos
        )
        {
            sendResponse(
                clientSocket,
                "400 Bad Request",
                "text/plain",
                "Bad Request"
            );

            closeSocket(clientSocket);
            continue;
        }

        string method =
            request.substr(
                0,
                firstSpace
            );

        string path =
            request.substr(
                firstSpace + 1,
                secondSpace - firstSpace - 1
            );

        cout << method << " " << path << endl;

        // ----------------------------------------------------
        // OPTIONS
        // ----------------------------------------------------

        if (method == "OPTIONS")
        {
            sendResponse(
                clientSocket,
                "204 No Content",
                "text/plain",
                ""
            );
        }

        // ----------------------------------------------------
        // LOGIN
        // ----------------------------------------------------

        else if (
            method == "POST" &&
            path == "/api/login"
        )
        {
            size_t bodyPos =
                request.find("\r\n\r\n");

            string body;

            if (bodyPos != string::npos)
                body = request.substr(bodyPos + 4);

            string username =
                getFormValue(body, "username");

            string password =
                getFormValue(body, "password");

            cout << "Login attempt: "
                 << username << endl;

            if (
                username == USERNAME &&
                password == PASSWORD
            )
            {
                string headers =
                    "Set-Cookie: matrix_session=" +
                    SESSION_TOKEN +
                    "; Path=/; HttpOnly; SameSite=Lax\r\n";

                sendResponse(
                    clientSocket,
                    "200 OK",
                    "application/json",
                    "{\"success\":true,"
                    "\"message\":\"Login successful\"}",
                    headers
                );
            }
            else
            {
                sendJson(
                    clientSocket,
                    "401 Unauthorized",
                    "{\"success\":false,"
                    "\"message\":\"Invalid username or password\"}"
                );
            }
        }

        // ----------------------------------------------------
        // LOGOUT
        // ----------------------------------------------------

        else if (
            method == "GET" &&
            path == "/api/logout"
        )
        {
            string headers =
                "Set-Cookie: matrix_session=;"
                " Path=/; Max-Age=0;"
                " HttpOnly; SameSite=Lax\r\n";

            sendResponse(
                clientSocket,
                "200 OK",
                "application/json",
                "{\"success\":true}",
                headers
            );
        }

        // ----------------------------------------------------
        // HEALTH
        // ----------------------------------------------------

        else if (
            method == "GET" &&
            path == "/api/health"
        )
        {
            sendJson(
                clientSocket,
                "200 OK",
                "{\"success\":true,"
                "\"status\":\"Backend is running\"}"
            );
        }

        // ----------------------------------------------------
        // CALCULATE
        // ----------------------------------------------------

        else if (
            method == "POST" &&
            path == "/api/calculate"
        )
        {
            handleCalculate(
                clientSocket,
                request
            );
        }

        // ----------------------------------------------------
        // HOME
        // ----------------------------------------------------

        else if (
            method == "GET" &&
            path == "/"
        )
        {
            string filename =
                isLoggedIn(request)
                    ? "frontend/index.html"
                    : "frontend/login.html";

            string html =
                readFile(filename);

            if (html.empty())
            {
                sendResponse(
                    clientSocket,
                    "500 Internal Server Error",
                    "text/plain",
                    "HTML file not found."
                );
            }
            else
            {
                sendResponse(
                    clientSocket,
                    "200 OK",
                    "text/html; charset=utf-8",
                    html
                );
            }
        }

        // ----------------------------------------------------
        // LOGIN PAGE
        // ----------------------------------------------------

        else if (
            method == "GET" &&
            path == "/login.html"
        )
        {
            string html =
                readFile("frontend/login.html");

            sendResponse(
                clientSocket,
                "200 OK",
                "text/html; charset=utf-8",
                html
            );
        }

        // ----------------------------------------------------
        // INDEX PAGE
        // ----------------------------------------------------

        else if (
            method == "GET" &&
            path == "/index.html"
        )
        {
            string filename =
                isLoggedIn(request)
                    ? "frontend/index.html"
                    : "frontend/login.html";

            string html =
                readFile(filename);

            sendResponse(
                clientSocket,
                "200 OK",
                "text/html; charset=utf-8",
                html
            );
        }

        // ----------------------------------------------------
        // CSS
        // ----------------------------------------------------

        else if (
            method == "GET" &&
            path == "/style.css"
        )
        {
            string css =
                readFile("frontend/style.css");

            sendResponse(
                clientSocket,
                "200 OK",
                "text/css",
                css
            );
        }

        // ----------------------------------------------------
        // JAVASCRIPT
        // ----------------------------------------------------

        else if (
            method == "GET" &&
            path == "/script.js"
        )
        {
            string js =
                readFile("frontend/script.js");

            sendResponse(
                clientSocket,
                "200 OK",
                "application/javascript",
                js
            );
        }

        // ----------------------------------------------------
        // 404
        // ----------------------------------------------------

        else
        {
            sendResponse(
                clientSocket,
                "404 Not Found",
                "text/plain",
                "404 - Page not found"
            );
        }

        closeSocket(clientSocket);
    }

    closeSocket(serverSocket);
    stopNetworking();

    return 0;
}
