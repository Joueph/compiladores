import http.server
import socketserver
import subprocess
import urllib.parse
import os
import logging
import sys

# --- Obtém o caminho absoluto do diretório do script ---
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# CONFIGURAÇÕES
PORTA = 8000
EXECUTAVEL_COMPILADOR = os.path.join(SCRIPT_DIR, "analisador")
EXECUTAVEL_VM = os.path.join(SCRIPT_DIR, "vm")

if sys.platform == "win32":
    EXECUTAVEL_COMPILADOR += ".exe"
    EXECUTAVEL_VM += ".exe"

ARQUIVO_ENTRADA = os.path.join(SCRIPT_DIR, "teste.txt")
ARQUIVO_SAIDA_MVD = os.path.join(SCRIPT_DIR, "programa.mvd")
ARQUIVO_ESTADO_VM = os.path.join(SCRIPT_DIR, "vm_state.bin")

# --- CONFIGURAÇÃO DE LOGS ---
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - [%(levelname)s] - %(message)s',
    handlers=[
        logging.StreamHandler()  # Envia logs para o console
    ]
)

# Códigos de saída da VM
VM_EXIT_HALT = 0
VM_EXIT_INPUT_REQUIRED = 10

# HTML da Interface
HTML_PAGE = """
<!DOCTYPE html> 
<html lang="pt-br">
<head>
    <meta charset="UTF-8">
    <title>Compilador - Web IDE</title>
    <style>
        body {{ font-family: 'Segoe UI', sans-serif; background-color: #f4f4f9; padding: 20px; margin: 0;}}
        .container {{ display: flex; gap: 20px; max-width: 1400px; margin: 0 auto; height: 90vh; }}
        .box {{ flex: 1; display: flex; flex-direction: column; }}
        textarea {{ padding: 10px; font-family: 'Consolas', monospace; font-size: 14px; border: 1px solid #ccc; border-radius: 5px; resize: none; background-color: #fff; }}
        #codigo-fonte {{ flex: 1; }}
        #output-mvd {{ flex: 1; background-color: #e9ecef; color: #495057; min-height: 100px; }}
        #output-console {{ flex: 2; background-color: #1e1e1e; color: #00ff00; line-height: 1.4; }} /* Aumentado para flex: 2 */
        .header-box {{ display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px; }}
        .input-group {{ display: flex; flex-direction: column; gap: 10px; flex: 2; }} /* Aumentado para flex: 2 */
        .header-box h2 {{ margin: 0; color: #333; }}
        .header-box .buttons button {{ margin-left: 10px; }}
        button {{ padding: 10px 15px; border: none; font-size: 14px; cursor: pointer; border-radius: 5px; font-weight: bold; }}
        .btn-submit {{ background-color: #28a745; color: white; }}
        .btn-submit:hover {{ background-color: #218838; }}
        .btn-clear {{ background-color: #6c757d; color: white; }}
        .btn-clear:hover {{ background-color: #5a6268; }}
        .btn-download {{ background-color: #007bff; color: white; }}
        .btn-download:hover {{ background-color: #0069d9; }}
        .output-container {{ flex: 1; display: flex; flex-direction: column; gap: 10px; }}
        #vm-input-container {{ display: {vm_input_display}; }} /* Controla visibilidade */
        #vm-input-field {{ padding: 8px; border: 1px solid #007bff; border-radius: 5px; font-family: 'Consolas', monospace;}}
        .console-label {{ display: flex; justify-content: space-between; align-items: center; }}
        .status-badge {{ background-color: #ffc107; color: #333; padding: 5px 10px; border-radius: 12px; font-size: 12px; font-weight: bold;}}
        h2 {{ margin-top: 0; color: #333; }}
    </style>
</head>
<body>
    <form action="/" method="POST" id="formCompiler" style="display:contents;">
        <div class="container">
        <div class="box">
            <div class="header-box">
                <h2>Código Fonte (LPD)</h2>
                <div class="buttons">
                    <button type="button" id="clear-btn" class="btn-clear">Limpar</button>
                    <button type="submit" name="action" value="run" class="btn-submit">{submit_button_text}</button>
                </div>
            </div>
            <textarea id="codigo-fonte" name="codigo" placeholder="Digite seu código aqui...">{codigo_anterior}</textarea>
        </div>
        <div class="box">
            <div class="header-box">
                <h2>Código de Máquina (.mvd)</h2>
                <div id="download-area">{download_link}</div>
            </div>
            <textarea id="output-mvd" readonly>{codigo_mvd}</textarea>
            <div class="input-group">
                <div id="vm-input-container">
                    <label for="vm-input-field"><b>{vm_input_label}</b></label>
                    <input type="text" id="vm-input-field" name="vm_input" placeholder="Digite um valor e continue..." autocomplete="off">
                </div>
                <div class="console-label"><h2>Console</h2><span class="status-badge" style="display:{status_badge_display}">{status_badge_text}</span></div>
                <textarea id="output-console" readonly>{resultado}</textarea>
            </div>
            <input type="hidden" name="console_output" value="{resultado}">
        </div>
    </div>
    </form>
    <script>
        document.getElementById('clear-btn').addEventListener('click', function() {{
            document.getElementById('codigo-fonte').value = '';
            document.getElementById('output-mvd').value = '';
            document.getElementById('output-console').value = 'Pronto para compilar...';
            // Envia um post para limpar o estado no servidor
            fetch('/', {{ method: 'POST', headers: {{'Content-Type': 'application/x-www-form-urlencoded'}}, body: 'action=clear' }});
            document.getElementById('download-area').innerHTML = '';
        }});
    </script>
</body>
</html>
"""

DOWNLOAD_FILENAME = "programa.mvd"

class CompilerHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        logging.info(f"Requisição GET recebida para: {self.path}")
        # Rota para baixar o arquivo .mvd
        if self.path == f"/{ARQUIVO_SAIDA_MVD}":
            if os.path.exists(ARQUIVO_SAIDA_MVD):
                self.send_response(200)
                self.send_header("Content-type", "application/octet-stream")
                self.send_header("Content-Disposition", f'attachment; filename="{DOWNLOAD_FILENAME}"')
                self.end_headers()
                with open(ARQUIVO_SAIDA_MVD, 'rb') as f:
                    self.wfile.write(f.read())
                logging.info(f"Arquivo '{ARQUIVO_SAIDA_MVD}' enviado para download.")
            else:
                logging.warning(f"Tentativa de baixar arquivo inexistente: '{ARQUIVO_SAIDA_MVD}'")
                self.send_response(404)
                self.end_headers()
            return

        # Página Principal
        logging.info("Servindo página principal HTML.")
        self.send_response(200)
        self.send_header("Content-type", "text/html; charset=utf-8")
        self.end_headers()
        # Estado inicial da página
        self.wfile.write(HTML_PAGE.format(
            codigo_anterior="", 
            resultado="Pronto para compilar...",
            codigo_mvd="",
            download_link="",
            submit_button_text="Compilar e Executar",
            vm_input_display="none",
            vm_input_label="Entrada necessária:", # Padrão
            status_badge_display="none",
            status_badge_text=""
        ).encode('utf-8'))

    def do_POST(self):
        logging.info("Requisição POST recebida para compilar e executar.")
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length).decode('utf-8')
        parsed_data = urllib.parse.parse_qs(post_data)
        codigo_fonte = parsed_data.get('codigo', [''])[0]
        action = parsed_data.get('action', [''])[0]
        vm_input = parsed_data.get('vm_input', [''])[0]

        # Ação de limpar o estado
        if action == 'clear':
            if os.path.exists(ARQUIVO_ESTADO_VM):
                os.remove(ARQUIVO_ESTADO_VM)
                logging.info("Estado da VM limpo.")
            self.send_response(204) # No Content
            self.end_headers()
            return

        resultado_texto = ""
        codigo_mvd_texto = ""
        download_link_html = ""
        vm_needs_input = False
        
        # Se não for uma continuação, é uma nova compilação
        is_continuation = os.path.exists(ARQUIVO_ESTADO_VM) and action == 'run'

        if not is_continuation:
            # Limpa estado anterior antes de compilar
            if os.path.exists(ARQUIVO_ESTADO_VM):
                os.remove(ARQUIVO_ESTADO_VM)

            # Salva o código fonte
            logging.info(f"Salvando código fonte em '{ARQUIVO_ENTRADA}'.")
            with open(ARQUIVO_ENTRADA, "w", encoding="utf-8") as f:
                f.write(codigo_fonte)
            
            try:
                # 1. Executa o COMPILADOR
                comando_compilador = [EXECUTAVEL_COMPILADOR, ARQUIVO_ENTRADA]
                logging.info(f"Executando compilador: {' '.join(comando_compilador)}")
                processo_compilacao = subprocess.run(
                    comando_compilador,
                    capture_output=True, text=True, check=False
                )
                logging.info(f"Compilador finalizado com código de saída: {processo_compilacao.returncode}")
                
                if processo_compilacao.stdout:
                    resultado_texto += "--- SAÍDA DA COMPILAÇÃO ---\n" + processo_compilacao.stdout + "\n"
                if processo_compilacao.stderr:
                    resultado_texto += "--- ERROS DE COMPILAÇÃO ---\n" + processo_compilacao.stderr + "\n"

                # Se a compilação falhou, para aqui
                if processo_compilacao.returncode != 0:
                    resultado_texto += "\n[Compilação Falhou - Execução abortada]"
                    self.render_page(codigo_fonte, resultado_texto, "", "", False)
                    return

            except Exception as e:
                logging.critical(f"Erro crítico ao compilar: {e}")
                self.render_page(codigo_fonte, f"Erro crítico no servidor: {e}", "", "", False)
                return

        # 2. Executa a MÁQUINA VIRTUAL (seja do início ou continuação)
        if not os.path.exists(ARQUIVO_SAIDA_MVD):
            self.render_page(codigo_fonte, "Arquivo .mvd não encontrado. A compilação falhou?", "", "", False)
            return

        try:
            # Lê o código MVD para exibição (sempre)
            with open(ARQUIVO_SAIDA_MVD, 'r', encoding='utf-8') as f_mvd:
                codigo_mvd_texto = f_mvd.read()
            download_link_html = f'<a href="/{ARQUIVO_SAIDA_MVD}" download><button type="button" class="btn-download">Baixar .mvd</button></a>'

            # Prepara e executa a VM
            comando_vm = [EXECUTAVEL_VM, ARQUIVO_SAIDA_MVD, "--step", ARQUIVO_ESTADO_VM]
            logging.info(f"Executando VM (interativo): {' '.join(comando_vm)}")
            logging.info(f"Input fornecido: '{vm_input}'")
            logging.info(f"Estado existe antes: {os.path.exists(ARQUIVO_ESTADO_VM)}")
            
            # Se for uma continuação, pega a saída anterior
            if is_continuation:
                resultado_texto = parsed_data.get('console_output', [''])[0]
                resultado_texto += f"> {vm_input}\n" # Mostra o input do usuário
                logging.info("Modo: CONTINUAÇÃO de execução")
            else:
                logging.info("Modo: NOVA execução")

            processo_vm = subprocess.run(
                comando_vm,
                capture_output=True, text=True, input=vm_input, check=False
            )
            logging.info(f"VM finalizada com código de saída: {processo_vm.returncode}")
            logging.info(f"Estado existe depois: {os.path.exists(ARQUIVO_ESTADO_VM)}")
            
            # Logs detalhados da VM
            logging.debug(f"VM stdout:\n---\n{processo_vm.stdout}\n---")
            if processo_vm.stderr:
                logging.warning(f"VM stderr:\n---\n{processo_vm.stderr}\n---")

            resultado_texto += processo_vm.stdout
            if processo_vm.stderr:
                resultado_texto += "\n[Erro na Execução]:\n" + processo_vm.stderr


            # Verifica se a VM está pedindo input para uma variável específica
            vm_input_var_name = None
            if "INPUT_REQUEST_VAR:" in processo_vm.stdout:
                for line in processo_vm.stdout.splitlines():
                    if line.startswith("INPUT_REQUEST_VAR:"):
                        vm_input_var_name = line.split(":", 1)[1]
                        break

            # Verifica o estado da VM
            if processo_vm.returncode == VM_EXIT_INPUT_REQUIRED:
                vm_needs_input = True
                logging.info("VM pausada, aguardando input.")
            elif processo_vm.returncode == VM_EXIT_HALT:
                logging.info("VM finalizou a execução (HLT).")
                if os.path.exists(ARQUIVO_ESTADO_VM): os.remove(ARQUIVO_ESTADO_VM)
            else: # Erro
                logging.error("VM encontrou um erro e parou.")
                if os.path.exists(ARQUIVO_ESTADO_VM): os.remove(ARQUIVO_ESTADO_VM)

        except Exception as e_vm:
            logging.error(f"Erro ao executar a VM: {e_vm}")
            resultado_texto += f"\nErro ao rodar VM: {e_vm}"
            if os.path.exists(ARQUIVO_ESTADO_VM): os.remove(ARQUIVO_ESTADO_VM)

        self.render_page(codigo_fonte, resultado_texto, codigo_mvd_texto, download_link_html, vm_needs_input, vm_input_var_name)

    def render_page(self, codigo, resultado, mvd, download_link, vm_needs_input, var_name=None):
        self.send_response(200)
        self.send_header("Content-type", "text/html; charset=utf-8")
        self.end_headers()
        vm_input_label = "Entrada necessária para 'leia':"

        if vm_needs_input:
            submit_text = "Continuar Execução"
            vm_input_display = "block"
            status_badge_display = "inline-block"
            status_badge_text = "Aguardando Entrada"
            if var_name:
                vm_input_label = f"Entrada necessária para var <b>({var_name})</b>:"
        else:
            submit_text = "Compilar e Executar"
            vm_input_display = "none"
            status_badge_display = "none"
            status_badge_text = ""

        self.wfile.write(HTML_PAGE.format(
            codigo_anterior=codigo, 
            resultado=resultado, 
            codigo_mvd=mvd,
            download_link=download_link,
            submit_button_text=submit_text,
            vm_input_display=vm_input_display,
            vm_input_label=vm_input_label,
            status_badge_display=status_badge_display,
            status_badge_text=status_badge_text
        ).encode('utf-8'))

if __name__ == "__main__":
    logging.info("--- Servidor Compilador + VM Iniciado ---")
    logging.info(f"Acesse: http://localhost:{PORTA}")
    server = socketserver.TCPServer(("0.0.0.0", PORTA), CompilerHandler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        logging.info("Servidor interrompido pelo usuário (Ctrl+C).")
        server.server_close()
        logging.info("Servidor encerrado.")