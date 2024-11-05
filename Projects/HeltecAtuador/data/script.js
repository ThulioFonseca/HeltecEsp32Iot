function toggleRelay(relayNum, state) {
  const xhr = new XMLHttpRequest();
  xhr.open("GET", `/relay?relay=${relayNum}&state=${state ? "on" : "off"}`, true);
  xhr.send();
}

window.onload = function() {
  // Função para atualizar o status dos relés
  function updateRelayStatus() {
    const xhr = new XMLHttpRequest();
    xhr.open("GET", "/status", true);
    xhr.onload = function() {
      if (xhr.status === 200) {
        const status = JSON.parse(xhr.responseText);
        for (let i = 0; i < 7; i++) {
          document.getElementById(`relay${i + 1}`).checked = status[`relay${i}`];
        }
      }
    };
    xhr.send();
  }

  // Chama a função uma vez ao carregar a página
  updateRelayStatus();

  // Atualiza o status dos relés a cada 5 segundos
  setInterval(updateRelayStatus, 5000);
};