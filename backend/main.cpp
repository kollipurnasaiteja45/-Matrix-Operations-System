#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <stdexcept>

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// ============================================================
// CONFIGURATION
// ============================================================

const string USERNAME = "admin";
const string PASSWORD = "admin123";

const string SESSION_TOKEN = "MATRIXLAB_LOGIN_2026";

const int PORT = 8080;

// ============================================================
// MATRIX TYPE
// ============================================================

using Matrix = vector<vector<double>>;

// ============================================================
// FILE READING
// ============================================================

string readFile(const string& filename)
{
    ifstream file(filename);

    if (!file.is_open())
    {
        return "";
    }

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
        if (value[i] == '%')
        {
            if (i + 2 < value.length())
            {
                string hex = value.substr(i + 1, 2);

                char ch = static_cast<char>(
                    strtol(hex.c_str(), nullptr, 16)
                );

                result += ch;
                i += 2;
            }
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
    {
        return "";
    }

    start += searchKey.length();

    size_t end = body.find('&', start);

    if (end == string::npos)
    {
        end = body.length();
    }

    return urlDecode(body.substr(start, end - start));
}

// ============================================================
// LOGIN CHECK
// ============================================================

bool isLoggedIn(const string& request)
{
    string search = "matrix_session=" + SESSION_TOKEN;

    return request.find(search) != string::npos;
}

// ============================================================
// HTTP RESPONSE
// ============================================================

void sendResponse(
    SOCKET client,
    const string& status,
    const string& contentType,
    const string& body,
    const string& extraHeaders = ""
)
{
    string response;

    response =
        "HTTP/1.1 " + status + "\r\n"
        "Content-Type: " + contentType + "\r\n"
        "Content-Length: " + to_string(body.size()) + "\r\n"
        "Access-Control-Allow-Origin: http://localhost:8080\r\n"
        "Access-Control-Allow-Credentials: true\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Connection: close\r\n"
        + extraHeaders +
        "\r\n" +
        body;

    send(
        client,
        response.c_str(),
        static_cast<int>(response.size()),
        0
    );
}

// ============================================================
// JSON RESPONSE
// ============================================================

void sendJson(
    SOCKET client,
    const string& status,
    const string& json
)
{
    sendResponse(
        client,
        status,
        "application/json",
        json
    );
}

// ============================================================
// MATRIX ADDITION
// ============================================================

Matrix addMatrix(
    const Matrix& A,
    const Matrix& B
)
{
    int rows = static_cast<int>(A.size());
    int cols = static_cast<int>(A[0].size());

    Matrix result(rows, vector<double>(cols));

    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            result[i][j] = A[i][j] + B[i][j];
        }
    }

    return result;
}

// ============================================================
// MATRIX SUBTRACTION
// ============================================================

Matrix subtractMatrix(
    const Matrix& A,
    const Matrix& B
)
{
    int rows = static_cast<int>(A.size());
    int cols = static_cast<int>(A[0].size());

    Matrix result(rows, vector<double>(cols));

    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            result[i][j] = A[i][j] - B[i][j];
        }
    }

    return result;
}

// ============================================================
// MATRIX MULTIPLICATION
// ============================================================

Matrix multiplyMatrix(
    const Matrix& A,
    const Matrix& B
)
{
    int rowsA = static_cast<int>(A.size());
    int colsA = static_cast<int>(A[0].size());

    int rowsB = static_cast<int>(B.size());
    int colsB = static_cast<int>(B[0].size());

    if (colsA != rowsB)
    {
        throw runtime_error(
            "For multiplication, columns of Matrix A "
            "must equal rows of Matrix B."
        );
    }

    Matrix result(
        rowsA,
        vector<double>(colsB, 0)
    );

    for (int i = 0; i < rowsA; i++)
    {
        for (int j = 0; j < colsB; j++)
        {
            for (int k = 0; k < colsA; k++)
            {
                result[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    return result;
}

// ============================================================
// TRANSPOSE
// ============================================================

Matrix transposeMatrix(const Matrix& A)
{
    int rows = static_cast<int>(A.size());
    int cols = static_cast<int>(A[0].size());

    Matrix result(
        cols,
        vector<double>(rows)
    );

    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            result[j][i] = A[i][j];
        }
    }

    return result;
}

// ============================================================
// DETERMINANT
// ============================================================

double determinant(const Matrix& A)
{
    int n = static_cast<int>(A.size());

    if (n == 1)
    {
        return A[0][0];
    }

    if (n == 2)
    {
        return
            A[0][0] * A[1][1]
            -
            A[0][1] * A[1][0];
    }

    double det = 0;

    for (int col = 0; col < n; col++)
    {
        Matrix subMatrix;

        for (int i = 1; i < n; i++)
        {
            vector<double> row;

            for (int j = 0; j < n; j++)
            {
                if (j != col)
                {
                    row.push_back(A[i][j]);
                }
            }

            subMatrix.push_back(row);
        }

        double sign = (col % 2 == 0) ? 1 : -1;

        det +=
            sign *
            A[0][col] *
            determinant(subMatrix);
    }

    return det;
}

// ============================================================
// MATRIX INVERSE
// ============================================================

Matrix inverseMatrix(const Matrix& A)
{
    int n = static_cast<int>(A.size());

    double det = determinant(A);

    if (fabs(det) < 1e-10)
    {
        throw runtime_error(
            "Matrix inverse does not exist because "
            "the determinant is zero."
        );
    }

    Matrix augmented(
        n,
        vector<double>(2 * n)
    );

    // Create [A | I]
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            augmented[i][j] = A[i][j];
        }

        for (int j = 0; j < n; j++)
        {
            augmented[i][j + n] =
                (i == j) ? 1 : 0;
        }
    }

    // Gauss-Jordan elimination
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
            {
                throw runtime_error(
                    "Matrix inverse does not exist."
                );
            }

            swap(
                augmented[i],
                augmented[swapRow]
            );

            pivot = augmented[i][i];
        }

        // Divide pivot row
        for (int j = 0; j < 2 * n; j++)
        {
            augmented[i][j] /= pivot;
        }

        // Eliminate other rows
        for (int k = 0; k < n; k++)
        {
            if (k == i)
            {
                continue;
            }

            double factor = augmented[k][i];

            for (int j = 0; j < 2 * n; j++)
            {
                augmented[k][j] -=
                    factor * augmented[i][j];
            }
        }
    }

    Matrix result(
        n,
        vector<double>(n)
    );

    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            result[i][j] =
                augmented[i][j + n];
        }
    }

    return result;
}

// ============================================================
// SCALAR MULTIPLICATION
// ============================================================

Matrix scalarMultiply(
    const Matrix& A,
    double scalar
)
{
    int rows = static_cast<int>(A.size());
    int cols = static_cast<int>(A[0].size());

    Matrix result(
        rows,
        vector<double>(cols)
    );

    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            result[i][j] =
                A[i][j] * scalar;
        }
    }

    return result;
}

// ============================================================
// TRACE
// ============================================================

double traceMatrix(const Matrix& A)
{
    int rows = static_cast<int>(A.size());
    int cols = static_cast<int>(A[0].size());

    if (rows != cols)
    {
        throw runtime_error(
            "Trace is defined only for a square matrix."
        );
    }

    double result = 0;

    for (int i = 0; i < rows; i++)
    {
        result += A[i][i];
    }

    return result;
}

// ============================================================
// IDENTITY MATRIX
// ============================================================

Matrix identityMatrix(int n)
{
    Matrix result(
        n,
        vector<double>(n, 0)
    );

    for (int i = 0; i < n; i++)
    {
        result[i][i] = 1;
    }

    return result;
}

// ============================================================
// EXTRACT NUMBERS FROM JSON
// ============================================================

vector<double> extractNumbers(
    const string& text
)
{
    vector<double> numbers;

    string current;

    for (size_t i = 0; i < text.length(); i++)
    {
        char c = text[i];

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
        else
        {
            if (!current.empty())
            {
                try
                {
                    numbers.push_back(
                        stod(current)
                    );
                }
                catch (...)
                {
                }

                current.clear();
            }
        }
    }

    if (!current.empty())
    {
        try
        {
            numbers.push_back(
                stod(current)
            );
        }
        catch (...)
        {
        }
    }

    return numbers;
}

// ============================================================
// GET JSON VALUE
// ============================================================

string getJsonValue(
    const string& json,
    const string& key
)
{
    string search =
        "\"" + key + "\"";

    size_t pos =
        json.find(search);

    if (pos == string::npos)
    {
        return "";
    }

    pos =
        json.find(':', pos);

    if (pos == string::npos)
    {
        return "";
    }

    pos++;

    while (
        pos < json.length() &&
        (json[pos] == ' ' ||
         json[pos] == '\t' ||
         json[pos] == '\r' ||
         json[pos] == '\n')
    )
    {
        pos++;
    }

    if (
        pos < json.length() &&
        json[pos] == '"'
    )
    {
        pos++;

        size_t end = pos;

        while (end < json.length())
        {
            if (
                json[end] == '"' &&
                json[end - 1] != '\\'
            )
            {
                break;
            }

            end++;
        }

        return json.substr(
            pos,
            end - pos
        );
    }

    size_t end =
        json.find_first_of(
            ",}",
            pos
        );

    if (end == string::npos)
    {
        end = json.length();
    }

    return json.substr(
        pos,
        end - pos
    );
}

// ============================================================
// GET JSON NUMBER
// ============================================================

double getJsonNumber(
    const string& json,
    const string& key,
    double defaultValue = 0
)
{
    string value =
        getJsonValue(json, key);

    if (value.empty())
    {
        return defaultValue;
    }

    try
    {
        return stod(value);
    }
    catch (...)
    {
        return defaultValue;
    }
}

// ============================================================
// PARSE MATRIX FROM JSON
// ============================================================

Matrix parseMatrix(
    const string& json,
    const string& key,
    int rows,
    int cols
)
{
    string search =
        "\"" + key + "\"";

    size_t keyPos =
        json.find(search);

    if (keyPos == string::npos)
    {
        throw runtime_error(
            "Matrix " + key + " not found."
        );
    }

    size_t start =
        json.find('[', keyPos);

    if (start == string::npos)
    {
        throw runtime_error(
            "Invalid matrix format."
        );
    }

    int depth = 0;
    size_t end = start;

    for (size_t i = start; i < json.length(); i++)
    {
        if (json[i] == '[')
        {
            depth++;
        }
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
    {
        throw runtime_error(
            "Invalid matrix brackets."
        );
    }

    string matrixText =
        json.substr(
            start,
            end - start + 1
        );

    vector<double> numbers =
        extractNumbers(matrixText);

    if (
        static_cast<int>(numbers.size())
        != rows * cols
    )
    {
        throw runtime_error(
            "Invalid number of matrix values."
        );
    }

    Matrix matrix(
        rows,
        vector<double>(cols)
    );

    int index = 0;

    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            matrix[i][j] =
                numbers[index++];
        }
    }

    return matrix;
}

// ============================================================
// FORMAT NUMBER
// ============================================================

string formatNumber(double value)
{
    if (fabs(value) < 1e-10)
    {
        value = 0;
    }

    ostringstream out;

    out.fixed;
    out.precision(4);

    out << value;

    string result =
        out.str();

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

// ============================================================
// MATRIX TO TEXT
// ============================================================

string matrixToText(
    const Matrix& matrix
)
{
    string result;

    for (size_t i = 0;
         i < matrix.size();
         i++)
    {
        result += "[ ";

        for (size_t j = 0;
             j < matrix[i].size();
             j++)
        {
            result +=
                formatNumber(
                    matrix[i][j]
                );

            if (
                j + 1 <
                matrix[i].size()
            )
            {
                result += "    ";
            }
        }

        result += " ]";

        if (
            i + 1 <
            matrix.size()
        )
        {
            result += "\n";
        }
    }

    return result;
}

// ============================================================
// ESCAPE JSON STRING
// ============================================================

string escapeJsonString(
    const string& value
)
{
    string result;

    for (char c : value)
    {
        if (c == '\\')
        {
            result += "\\\\";
        }
        else if (c == '"')
        {
            result += "\\\"";
        }
        else if (c == '\n')
        {
            result += "\\n";
        }
        else if (c == '\r')
        {
            result += "\\r";
        }
        else if (c == '\t')
        {
            result += "\\t";
        }
        else
        {
            result += c;
        }
    }

    return result;
}

// ============================================================
// CALCULATE API
// ============================================================

void handleCalculate(
    SOCKET client,
    const string& request
)
{
    // Authentication
    if (!isLoggedIn(request))
    {
        sendJson(
            client,
            "401 Unauthorized",
            "{\"success\":false,"
            "\"message\":\"Please login first.\"}"
        );

        return;
    }

    // Find request body
    size_t bodyPos =
        request.find("\r\n\r\n");

    if (bodyPos == string::npos)
    {
        sendJson(
            client,
            "400 Bad Request",
            "{\"success\":false,"
            "\"message\":\"Request body missing.\"}"
        );

        return;
    }

    string body =
        request.substr(bodyPos + 4);

    try
    {
        // ----------------------------------------------------
        // Read operation
        // ----------------------------------------------------

        string operation =
            getJsonValue(
                body,
                "operation"
            );

        int rowsA =
            static_cast<int>(
                getJsonNumber(
                    body,
                    "rowsA",
                    2
                )
            );

        int colsA =
            static_cast<int>(
                getJsonNumber(
                    body,
                    "colsA",
                    2
                )
            );

        int rowsB =
            static_cast<int>(
                getJsonNumber(
                    body,
                    "rowsB",
                    2
                )
            );

        int colsB =
            static_cast<int>(
                getJsonNumber(
                    body,
                    "colsB",
                    2
                )
            );

        double scalar =
            getJsonNumber(
                body,
                "scalar",
                1
            );

        // ----------------------------------------------------
        // Validate dimensions
        // ----------------------------------------------------

        if (
            rowsA <= 0 ||
            colsA <= 0 ||
            rowsA > 20 ||
            colsA > 20
        )
        {
            throw runtime_error(
                "Matrix A dimensions must be "
                "between 1 and 20."
            );
        }

        // ----------------------------------------------------
        // Parse Matrix A
        // ----------------------------------------------------

        Matrix A =
            parseMatrix(
                body,
                "matrixA",
                rowsA,
                colsA
            );

        Matrix result;

        // ====================================================
        // ADDITION
        // ====================================================

        if (operation == "addition")
        {
            if (
                rowsA != rowsB ||
                colsA != colsB
            )
            {
                throw runtime_error(
                    "For addition, both matrices "
                    "must have the same dimensions."
                );
            }

            Matrix B =
                parseMatrix(
                    body,
                    "matrixB",
                    rowsB,
                    colsB
                );

            result =
                addMatrix(A, B);
        }

        // ====================================================
        // SUBTRACTION
        // ====================================================

        else if (operation == "subtraction")
        {
            if (
                rowsA != rowsB ||
                colsA != colsB
            )
            {
                throw runtime_error(
                    "For subtraction, both matrices "
                    "must have the same dimensions."
                );
            }

            Matrix B =
                parseMatrix(
                    body,
                    "matrixB",
                    rowsB,
                    colsB
                );

            result =
                subtractMatrix(A, B);
        }

        // ====================================================
        // MULTIPLICATION
        // ====================================================

        else if (operation == "multiplication")
        {
            Matrix B =
                parseMatrix(
                    body,
                    "matrixB",
                    rowsB,
                    colsB
                );

            result =
                multiplyMatrix(A, B);
        }

        // ====================================================
        // TRANSPOSE
        // ====================================================

        else if (operation == "transpose")
        {
            result =
                transposeMatrix(A);
        }

        // ====================================================
        // INVERSE
        // ====================================================

        else if (operation == "inverse")
        {
            if (rowsA != colsA)
            {
                throw runtime_error(
                    "Inverse requires a square matrix."
                );
            }

            result =
                inverseMatrix(A);
        }

        // ====================================================
        // SCALAR MULTIPLICATION
        // ====================================================

        else if (operation == "scalar")
        {
            result =
                scalarMultiply(
                    A,
                    scalar
                );
        }

        // ====================================================
        // IDENTITY MATRIX
        // ====================================================

        else if (operation == "identity")
        {
            if (rowsA != colsA)
            {
                throw runtime_error(
                    "Identity matrix requires "
                    "a square size."
                );
            }

            result =
                identityMatrix(rowsA);
        }

        // ====================================================
        // DETERMINANT
        // ====================================================

        else if (operation == "determinant")
        {
            if (rowsA != colsA)
            {
                throw runtime_error(
                    "Determinant requires "
                    "a square matrix."
                );
            }

            double det =
                determinant(A);

            string response =
                "{\"success\":true,"
                "\"type\":\"number\","
                "\"result\":\"" +
                formatNumber(det) +
                "\"}";

            sendJson(
                client,
                "200 OK",
                response
            );

            return;
        }

        // ====================================================
        // TRACE
        // ====================================================

        else if (operation == "trace")
        {
            double trace =
                traceMatrix(A);

            string response =
                "{\"success\":true,"
                "\"type\":\"number\","
                "\"result\":\"" +
                formatNumber(trace) +
                "\"}";

            sendJson(
                client,
                "200 OK",
                response
            );

            return;
        }

        // ====================================================
        // INVALID OPERATION
        // ====================================================

        else
        {
            throw runtime_error(
                "Unknown matrix operation."
            );
        }

        // ====================================================
        // MATRIX RESPONSE
        // ====================================================

        string resultText =
            matrixToText(result);

        // IMPORTANT:
        // Escape ONLY the matrix string.
        // Do NOT escape the complete JSON object.

        string escapedResult =
            escapeJsonString(
                resultText
            );

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
            escapeJsonString(
                e.what()
            );

        string response =
            "{\"success\":false,"
            "\"message\":\"" +
            message +
            "\"}";

        sendJson(
            client,
            "400 Bad Request",
            response
        );
    }
}

// ============================================================
// MAIN SERVER
// ============================================================

int main()
{
    WSADATA wsaData;

    if (
        WSAStartup(
            MAKEWORD(2, 2),
            &wsaData
        ) != 0
    )
    {
        cerr
            << "WSAStartup failed."
            << endl;

        return 1;
    }

    SOCKET serverSocket =
        socket(
            AF_INET,
            SOCK_STREAM,
            0
        );

    if (
        serverSocket ==
        INVALID_SOCKET
    )
    {
        cerr
            << "Socket creation failed."
            << endl;

        WSACleanup();

        return 1;
    }

    // Allow address reuse
    int opt = 1;

    setsockopt(
        serverSocket,
        SOL_SOCKET,
        SO_REUSEADDR,
        reinterpret_cast<char*>(&opt),
        sizeof(opt)
    );

    sockaddr_in serverAddress{};

    serverAddress.sin_family =
        AF_INET;

    serverAddress.sin_addr.s_addr =
        inet_addr("127.0.0.1");

    serverAddress.sin_port =
        htons(PORT);

    if (
        bind(
            serverSocket,
            reinterpret_cast<sockaddr*>(
                &serverAddress
            ),
            sizeof(serverAddress)
        ) == SOCKET_ERROR
    )
    {
        cerr
            << "Bind failed."
            << endl;

        closesocket(serverSocket);
        WSACleanup();

        return 1;
    }

    if (
        listen(
            serverSocket,
            10
        ) == SOCKET_ERROR
    )
    {
        cerr
            << "Listen failed."
            << endl;

        closesocket(serverSocket);
        WSACleanup();

        return 1;
    }

    cout
        << "====================================="
        << endl;

    cout
        << "      MATRIXLAB C++ SERVER"
        << endl;

    cout
        << "====================================="
        << endl;

    cout
        << "Server running at:"
        << endl;

    cout
        << "http://localhost:8080"
        << endl;

    cout
        << endl;

    cout
        << "Username: admin"
        << endl;

    cout
        << "Password: admin123"
        << endl;

    cout
        << endl;

    cout
        << "Waiting for connections..."
        << endl;

    cout
        << "====================================="
        << endl;

    // ========================================================
    // SERVER LOOP
    // ========================================================

    while (true)
    {
        sockaddr_in clientAddress{};

        int clientSize =
            sizeof(clientAddress);

        SOCKET clientSocket =
            accept(
                serverSocket,
                reinterpret_cast<sockaddr*>(
                    &clientAddress
                ),
                &clientSize
            );

        if (
            clientSocket ==
            INVALID_SOCKET
        )
        {
            continue;
        }

        // ----------------------------------------------------
        // Receive HTTP request
        // ----------------------------------------------------

        string request;

        char buffer[65536];

        int bytesReceived;

        do
        {
            bytesReceived =
                recv(
                    clientSocket,
                    buffer,
                    sizeof(buffer),
                    0
                );

            if (bytesReceived > 0)
            {
                request.append(
                    buffer,
                    bytesReceived
                );
            }

        } while (
            bytesReceived ==
            sizeof(buffer)
        );

        if (request.empty())
        {
            closesocket(clientSocket);
            continue;
        }

        // ----------------------------------------------------
        // Request Line
        // ----------------------------------------------------

        size_t firstSpace =
            request.find(' ');

        size_t secondSpace =
            request.find(
                ' ',
                firstSpace + 1
            );

        string method =
            request.substr(
                0,
                firstSpace
            );

        string path =
            request.substr(
                firstSpace + 1,
                secondSpace -
                firstSpace -
                1
            );

        cout
            << method
            << " "
            << path
            << endl;

        // ====================================================
        // OPTIONS
        // ====================================================

        if (method == "OPTIONS")
        {
            sendResponse(
                clientSocket,
                "204 No Content",
                "text/plain",
                ""
            );
        }

        // ====================================================
        // LOGIN
        // ====================================================

        else if (
            method == "POST" &&
            path == "/api/login"
        )
        {
            size_t bodyPos =
                request.find("\r\n\r\n");

            string body;

            if (
                bodyPos !=
                string::npos
            )
            {
                body =
                    request.substr(
                        bodyPos + 4
                    );
            }

            string username =
                getFormValue(
                    body,
                    "username"
                );

            string password =
                getFormValue(
                    body,
                    "password"
                );

            cout
                << "Login attempt:"
                << endl;

            cout
                << "Username = ["
                << username
                << "]"
                << endl;

            cout
                << "Password = ["
                << password
                << "]"
                << endl;

            if (
                username == USERNAME &&
                password == PASSWORD
            )
            {
                string headers =
                    "Set-Cookie: "
                    "matrix_session=" +
                    SESSION_TOKEN +
                    "; Path=/; HttpOnly\r\n";

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

        // ====================================================
        // LOGOUT
        // ====================================================

        else if (
            method == "GET" &&
            path == "/api/logout"
        )
        {
            string headers =
                "Set-Cookie: "
                "matrix_session=; "
                "Path=/; "
                "Max-Age=0; "
                "HttpOnly\r\n";

            sendResponse(
                clientSocket,
                "200 OK",
                "application/json",
                "{\"success\":true}",
                headers
            );
        }

        // ====================================================
        // HEALTH CHECK
        // ====================================================

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

        // ====================================================
        // MATRIX CALCULATION
        // ====================================================

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

        // ====================================================
        // HOME PAGE
        // ====================================================

        else if (
            method == "GET" &&
            path == "/"
        )
        {
            if (isLoggedIn(request))
            {
                string html =
                    readFile(
                        "frontend/index.html"
                    );

                if (html.empty())
                {
                    sendResponse(
                        clientSocket,
                        "500 Internal Server Error",
                        "text/plain",
                        "index.html not found."
                    );
                }
                else
                {
                    sendResponse(
                        clientSocket,
                        "200 OK",
                        "text/html",
                        html
                    );
                }
            }
            else
            {
                string html =
                    readFile(
                        "frontend/login.html"
                    );

                if (html.empty())
                {
                    sendResponse(
                        clientSocket,
                        "500 Internal Server Error",
                        "text/plain",
                        "login.html not found."
                    );
                }
                else
                {
                    sendResponse(
                        clientSocket,
                        "200 OK",
                        "text/html",
                        html
                    );
                }
            }
        }

        // ====================================================
        // LOGIN PAGE
        // ====================================================

        else if (
            method == "GET" &&
            path == "/login.html"
        )
        {
            string html =
                readFile(
                    "frontend/login.html"
                );

            sendResponse(
                clientSocket,
                "200 OK",
                "text/html",
                html
            );
        }

        // ====================================================
        // INDEX PAGE
        // ====================================================

        else if (
            method == "GET" &&
            path == "/index.html"
        )
        {
            if (!isLoggedIn(request))
            {
                string html =
                    readFile(
                        "frontend/login.html"
                    );

                sendResponse(
                    clientSocket,
                    "200 OK",
                    "text/html",
                    html
                );
            }
            else
            {
                string html =
                    readFile(
                        "frontend/index.html"
                    );

                sendResponse(
                    clientSocket,
                    "200 OK",
                    "text/html",
                    html
                );
            }
        }

        // ====================================================
        // CSS
        // ====================================================

        else if (
            method == "GET" &&
            path == "/style.css"
        )
        {
            string css =
                readFile(
                    "frontend/style.css"
                );

            sendResponse(
                clientSocket,
                "200 OK",
                "text/css",
                css
            );
        }

        // ====================================================
        // JAVASCRIPT
        // ====================================================

        else if (
            method == "GET" &&
            path == "/script.js"
        )
        {
            string js =
                readFile(
                    "frontend/script.js"
                );

            sendResponse(
                clientSocket,
                "200 OK",
                "application/javascript",
                js
            );
        }

        // ====================================================
        // 404
        // ====================================================

        else
        {
            sendResponse(
                clientSocket,
                "404 Not Found",
                "text/plain",
                "404 - Page not found"
            );
        }

        closesocket(clientSocket);
    }

    // ========================================================
    // CLEANUP
    // ========================================================

    closesocket(serverSocket);

    WSACleanup();

    return 0;
}