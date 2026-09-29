let currentOperation = "add";


// ======================================================
// PAGE LOAD
// ======================================================

document.addEventListener("DOMContentLoaded", function () {

    createGrid("A");
    createGrid("B");

    selectOperation("add");

    checkBackend();

    loadTheme();

});


// ======================================================
// SELECT OPERATION
// ======================================================

function selectOperation(operation) {

    currentOperation = operation;

    document.querySelectorAll(".operation-card").forEach(card => {
        card.classList.remove("active");
    });

    const selected =
        document.querySelector(
            `[data-operation="${operation}"]`
        );

    if (selected) {
        selected.classList.add("active");
    }


    const matrixB =
        document.getElementById("matrixBCard");

    const scalarBox =
        document.getElementById("scalarBox");

    const operator =
        document.getElementById("operatorSymbol");


    if (
        operation === "add" ||
        operation === "subtract" ||
        operation === "multiply"
    ) {

        matrixB.classList.remove("hidden");
        scalarBox.classList.add("hidden");

    }

    else if (operation === "scalar") {

        matrixB.classList.add("hidden");
        scalarBox.classList.remove("hidden");

    }

    else {

        matrixB.classList.add("hidden");
        scalarBox.classList.add("hidden");

    }


    if (operation === "add") {
        operator.textContent = "+";
    }

    else if (operation === "subtract") {
        operator.textContent = "−";
    }

    else if (operation === "multiply") {
        operator.textContent = "×";
    }

    else if (operation === "scalar") {
        operator.textContent = "×";
    }

    else {
        operator.textContent = "";
    }


    // For multiplication:
    // columns of A = rows of B

    if (operation === "multiply") {

        const colsA =
            parseInt(
                document.getElementById("colsA").value
            );

        document.getElementById("rowsB").value = colsA;

        createGrid("B");

    }

}


// ======================================================
// CREATE MATRIX GRID
// ======================================================

function createGrid(matrix) {

    let rows;
    let cols;
    let container;


    if (matrix === "A") {

        rows =
            parseInt(
                document.getElementById("rowsA").value
            );

        cols =
            parseInt(
                document.getElementById("colsA").value
            );

        container =
            document.getElementById("gridA");

    }

    else {

        rows =
            parseInt(
                document.getElementById("rowsB").value
            );

        cols =
            parseInt(
                document.getElementById("colsB").value
            );

        container =
            document.getElementById("gridB");

    }


    if (!rows || rows < 1)
        rows = 1;

    if (!cols || cols < 1)
        cols = 1;


    if (rows > 10)
        rows = 10;

    if (cols > 10)
        cols = 10;


    container.innerHTML = "";


    container.style.gridTemplateColumns =
        `repeat(${cols}, minmax(55px, 1fr))`;


    for (let i = 0; i < rows; i++) {

        for (let j = 0; j < cols; j++) {

            const input =
                document.createElement("input");


            input.type = "number";

            input.step = "any";

            input.value = "0";

            input.className = "matrix-cell";


            input.id =
                `${matrix}_${i}_${j}`;


            container.appendChild(input);

        }

    }

}


// ======================================================
// READ MATRIX
// ======================================================

function readMatrix(matrix) {

    let rows;
    let cols;


    if (matrix === "A") {

        rows =
            parseInt(
                document.getElementById("rowsA").value
            );

        cols =
            parseInt(
                document.getElementById("colsA").value
            );

    }

    else {

        rows =
            parseInt(
                document.getElementById("rowsB").value
            );

        cols =
            parseInt(
                document.getElementById("colsB").value
            );

    }


    const matrixData = [];


    for (let i = 0; i < rows; i++) {

        const row = [];


        for (let j = 0; j < cols; j++) {

            const input =
                document.getElementById(
                    `${matrix}_${i}_${j}`
                );


            if (!input) {

                throw new Error(
                    `Matrix ${matrix} input is missing.`
                );

            }


            row.push(
                Number(input.value)
            );

        }


        matrixData.push(row);

    }


    return matrixData;

}


// ======================================================
// CALCULATE
// ======================================================

async function calculate() {

    clearError();


    const resultBox =
        document.getElementById("result");


    resultBox.textContent =
        "Calculating...";


    try {

        const rowsA =
            parseInt(
                document.getElementById("rowsA").value
            );

        const colsA =
            parseInt(
                document.getElementById("colsA").value
            );


        const matrixA =
            readMatrix("A");


        let rowsB = 0;
        let colsB = 0;
        let matrixB = [];


        if (
            currentOperation === "add" ||
            currentOperation === "subtract" ||
            currentOperation === "multiply"
        ) {

            rowsB =
                parseInt(
                    document.getElementById("rowsB").value
                );

            colsB =
                parseInt(
                    document.getElementById("colsB").value
                );


            matrixB =
                readMatrix("B");

        }


        let scalar = 1;


        if (currentOperation === "scalar") {

            scalar =
                Number(
                    document.getElementById("scalar").value
                );

        }


        let operation;


        switch (currentOperation) {

            case "add":
                operation = "addition";
                break;

            case "subtract":
                operation = "subtraction";
                break;

            case "multiply":
                operation = "multiplication";
                break;

            case "transpose":
                operation = "transpose";
                break;

            case "determinant":
                operation = "determinant";
                break;

            case "inverse":
                operation = "inverse";
                break;

            case "scalar":
                operation = "scalar";
                break;

            case "trace":
                operation = "trace";
                break;

            case "identity":
                operation = "identity";
                break;

            default:
                throw new Error(
                    "Unknown operation."
                );

        }


        const requestData = {

            operation: operation,

            rowsA: rowsA,

            colsA: colsA,

            rowsB: rowsB,

            colsB: colsB,

            matrixA: matrixA,

            matrixB: matrixB,

            scalar: scalar

        };


        console.log(
            "Sending to C++:",
            requestData
        );


        const response =
            await fetch(
                "/api/calculate",
                {
                    method: "POST",

                    headers: {
                        "Content-Type":
                            "application/json"
                    },

                    credentials: "include",

                    body:
                        JSON.stringify(requestData)
                }
            );


        console.log(
            "HTTP status:",
            response.status
        );


        const text =
            await response.text();


        console.log(
            "Server response:",
            text
        );


        let data;


        try {

            data =
                JSON.parse(text);

        }

        catch (e) {

            throw new Error(
                "Invalid response from C++ server: " +
                text
            );

        }


        if (response.status === 401) {

            window.location.href = "/";

            return;

        }


        if (!data.success) {

            throw new Error(
                data.message ||
                "Calculation failed."
            );

        }


        let output =
            data.result;


        // Matrix result

        if (data.type === "matrix") {

            output =
                output.replace(
                    /\\n/g,
                    "\n"
                );

        }


        resultBox.textContent =
            output;


        addHistory(
            currentOperation,
            output
        );


    }

    catch (error) {

        console.error(
            "Calculation error:",
            error
        );


        resultBox.textContent =
            "Calculation failed.";


        showError(
            error.message
        );

    }

}


// ======================================================
// RESET
// ======================================================

function resetMatrices() {

    document.getElementById("rowsA").value = 2;

    document.getElementById("colsA").value = 2;

    document.getElementById("rowsB").value = 2;

    document.getElementById("colsB").value = 2;

    document.getElementById("scalar").value = 2;


    createGrid("A");

    createGrid("B");


    document.getElementById("result")
        .textContent =
        "No result yet.";


    clearError();


    selectOperation("add");

}


// ======================================================
// ERROR
// ======================================================

function showError(message) {

    document.getElementById("error")
        .textContent = message;

}


function clearError() {

    document.getElementById("error")
        .textContent = "";

}


// ======================================================
// COPY
// ======================================================

async function copyResult() {

    const result =
        document.getElementById("result")
            .textContent;


    if (
        !result ||
        result === "No result yet."
    ) {
        return;
    }


    try {

        await navigator.clipboard
            .writeText(result);


        const button =
            document.querySelector(
                ".copy-button"
            );


        const oldText =
            button.textContent;


        button.textContent =
            "Copied!";


        setTimeout(
            () => {
                button.textContent =
                    oldText;
            },
            1000
        );

    }

    catch (error) {

        console.error(error);

    }

}


// ======================================================
// HISTORY
// ======================================================

function addHistory(
    operation,
    result
) {

    const history =
        document.getElementById(
            "history"
        );


    const empty =
        history.querySelector(
            ".empty-history"
        );


    if (empty) {
        empty.remove();
    }


    const item =
        document.createElement(
            "div"
        );


    item.className =
        "history-item";


    const title =
        operation.charAt(0).toUpperCase() +
        operation.slice(1);


    item.innerHTML =
        `<strong>${title}</strong>
         <pre>${escapeHtml(result)}</pre>`;


    history.prepend(item);


    while (
        history.children.length > 5
    ) {

        history.removeChild(
            history.lastChild
        );

    }

}


// ======================================================
// ESCAPE HTML
// ======================================================

function escapeHtml(text) {

    const div =
        document.createElement("div");

    div.textContent = text;

    return div.innerHTML;

}


// ======================================================
// BACKEND STATUS
// ======================================================

async function checkBackend() {

    const status =
        document.getElementById(
            "status"
        );


    const dot =
        document.getElementById(
            "statusDot"
        );


    try {

        const response =
            await fetch(
                "/api/health",
                {
                    credentials: "include"
                }
            );


        const data =
            await response.json();


        if (
            response.ok &&
            data.success
        ) {

            status.textContent =
                "Backend Online";

            dot.style.background =
                "#16a34a";

        }

        else {

            status.textContent =
                "Backend Error";

            dot.style.background =
                "#dc2626";

        }

    }

    catch (error) {

        status.textContent =
            "Backend Offline";

        dot.style.background =
            "#dc2626";

        console.error(error);

    }

}


// ======================================================
// LOGOUT
// ======================================================

async function logout() {

    try {

        await fetch(
            "/api/logout",
            {
                method: "GET",
                credentials: "include"
            }
        );

    }

    catch (error) {

        console.error(error);

    }


    window.location.href = "/";

}


// ======================================================
// DARK MODE
// ======================================================

function toggleTheme() {

    document.body.classList.toggle(
        "dark"
    );


    const dark =
        document.body.classList.contains(
            "dark"
        );


    localStorage.setItem(
        "matrixlab-theme",
        dark ? "dark" : "light"
    );


    updateThemeButton();

}


function loadTheme() {

    const theme =
        localStorage.getItem(
            "matrixlab-theme"
        );


    if (theme === "dark") {

        document.body.classList.add(
            "dark"
        );

    }


    updateThemeButton();

}


function updateThemeButton() {

    const button =
        document.getElementById(
            "themeButton"
        );


    if (!button)
        return;


    if (
        document.body.classList.contains(
            "dark"
        )
    ) {

        button.textContent = "☀️";

    }

    else {

        button.textContent = "🌙";

    }

}