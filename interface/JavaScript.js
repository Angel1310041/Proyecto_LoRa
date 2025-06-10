function menubar() {
    const menu = document.getElementById('menuu');
    menu.classList.toggle('activo');
}

function mostrarPantallaCarga() {
    const pantallaCarga = document.getElementById('pantalla-carga');
    const loader = pantallaCarga.querySelector('.loader-bolas');
    pantallaCarga.style.display = 'flex';
    loader.classList.remove('stop-animation');
}

function ocultarPantallaCarga() {
    const pantallaCarga = document.getElementById('pantalla-carga');
    const loader = pantallaCarga.querySelector('.loader-bolas');
    pantallaCarga.style.display = 'none';
    loader.classList.add('stop-animation');
}

function showContent(seccion) {
    if (seccion === 'salir') {
        if (confirm("¿Desea realmente salir?")) {
            mostrarPantallaCarga();
            fetch('/reiniciar', { method: 'POST' })
                .then(() => setTimeout(() => {
                    ocultarPantallaCarga();
                    location.reload();
                }, 1000));
        }
        return;
    }

    mostrarPantallaCarga();

    const secciones = ['apartado-inicio', 'apartado-pruebas', 'apartado-parametros'];
    secciones.forEach(id => document.getElementById(id).style.display = 'none');
    document.getElementById('menuu').classList.remove('activo');

    setTimeout(() => {
        ocultarPantallaCarga();

        switch (seccion) {
            case 'inicio':
                document.getElementById('apartado-inicio').style.display = 'block';
                fetchAndDisplayParameters();
                break;
            case 'pruebas':
                document.getElementById('apartado-pruebas').style.display = 'block';
                break;
            case 'parametros':
                document.getElementById('apartado-parametros').style.display = 'block';
                break;
            default:
                console.warn(`Sección desconocida: ${seccion}`);
        }
    }, 1000);
}

function mostrarPantallaLora(numero) {
    console.log("Solicitando mostrar pantalla en la tarjeta:", numero);
    fetch("/mostrar-pantalla", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ numero })
    })
    .then(response => response.json())
    .then(data => {
        if (data.status) {
            console.log("Pantalla mostrada correctamente:", data.status);
        } else if (data.error) {
            alert("Error al solicitar pantalla: " + data.error);
        } else {
            console.log("Respuesta desconocida al solicitar pantalla:", data);
        }
    })
    .catch(error => {
        alert("Error de comunicación con el servidor al solicitar pantalla.");
        console.error("Error en fetch:", error);
    });
}

function fetchAndDisplayParameters() {
    console.log("Solicitando parámetros actuales desde el servidor...");
    fetch("/get-parametros")
        .then(response => {
            if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
            return response.json();
        })
        .then(data => {
            const resumenId = document.getElementById('resumen-id');
            const resumenZona = document.getElementById('resumen-zona');
            const resumenTipo = document.getElementById('resumen-tipo-sensor');

            const tipoSensorMap = {
                0: "0 - Gas LP",
                1: "1 - Humo",
                2: "2 - Movimiento",
                3: "3 - Puerta",
                4: "4 - Ventana",
                5: "5 - Cortina",
                6: "6 - Botón Físico/Pánico",
                7: "7 - Palanca"
            };

            const tipoSensorText = tipoSensorMap[data.tipo] || `Tipo desconocido (${data.tipo})`;

            if (resumenId && resumenZona && resumenTipo) {
                resumenId.textContent = data.id;
                resumenZona.textContent = data.zona;
                resumenTipo.textContent = tipoSensorText;
                console.log("Parámetros actualizados correctamente.");
            } else {
                console.error("No se encontraron todos los elementos de resumen.");
            }
        })
        .catch(error => {
            console.error("Error al obtener parámetros:", error);
        });
}


document.addEventListener('DOMContentLoaded', () => {
    fetchAndDisplayParameters();

    // Validación en tiempo real para ID de alarma (máximo 4 dígitos numéricos)
    const idAlarmaInput = document.getElementById('id-alarma');
    if (idAlarmaInput) {
        idAlarmaInput.addEventListener('input', function () {
            this.value = this.value.replace(/[^0-9]/g, '').slice(0, 4);
        });
    }

    // Validación en tiempo real para zona (máximo 3 dígitos numéricos, rango 1-510)
    const zonaInput = document.getElementById('zona');
    if (zonaInput) {
        zonaInput.addEventListener('input', function () {
            // Limita solo números y máximo 3 dígitos
            this.value = this.value.replace(/[^0-9]/g, '').slice(0, 3);
            // Además valida que no pase de 510
            if (this.value !== '' && parseInt(this.value) > 510) {
                this.value = '510';
            }
        });
    }

    const form = document.getElementById('form-parametros');
    if (form) {
        form.addEventListener('submit', function(e) {
            e.preventDefault();
            const idAlarma = document.getElementById('id-alarma').value.trim();
            const zona = document.getElementById('zona').value.trim();
            const tipoSensorSelect = document.getElementById('tipo-sensor');
            const tipoSensor = tipoSensorSelect.value.trim();
            const tipoSensorText = tipoSensorSelect.options[tipoSensorSelect.selectedIndex].text;

            // Validaciones finales antes de enviar
            if (idAlarma === '' || idAlarma.length > 4) {
                alert("ID de alarma inválido. Debe tener hasta 4 dígitos numéricos.");
                return;
            }
            if (zona === '' || isNaN(zona) || parseInt(zona) < 1 || parseInt(zona) > 510) {
                alert("Zona inválida. Debe ser un número entre 1 y 510.");
                return;
            }

            const parametros = {
                "id-alarma": idAlarma,
                "zona": zona,
                "tipo-sensor": tipoSensor
            };

            mostrarPantallaCarga();

            console.log("Enviando parámetros:", parametros);
            fetch("/guardar-parametros", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify(parametros)
            })
            .then(response => response.json())
            .then(data => {
                ocultarPantallaCarga();
                if (data.status) {
                    alert(data.status);
                    document.getElementById('resumen-id').textContent = idAlarma;
                    document.getElementById('resumen-zona').textContent = zona;
                    document.getElementById('resumen-tipo-sensor').textContent = tipoSensorText;

                    console.log("Resumen actualizado.");
                } else if (data.error) {
                    alert("Error: " + data.error);
                } else {
                    alert("Respuesta desconocida del servidor.");
                }
            })
            .catch(error => {
                ocultarPantallaCarga();
                alert("Error de comunicación con el servidor.");
                console.error("Error en fetch:", error);
            });
        });
    }

    const btnEnviarRF = document.getElementById('btn-enviar-rf');
    if (btnEnviarRF) {
        btnEnviarRF.addEventListener('click', () => {
            console.log("Enviando señal RF de prueba...");
            fetch("/enviar-rf-prueba", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({})
            })
            .then(response => response.json())
            .then(data => {
                if (data.status) {
                    alert(data.status);
                } else if (data.error) {
                    alert("Error: " + data.error);
                } else {
                    alert("Respuesta desconocida del servidor.");
                }
            })
            .catch(error => {
                alert("Error de comunicación con el servidor al enviar RF.");
                console.error("Error en fetch:", error);
            });
        });
    }
});