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

# --- CONFIGURAÇÃO DE LOGS ---
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - [%(levelname)s] - %(message)s',
    handlers=[
        logging.StreamHandler()  # Envia logs para o console
    ]
)

# HTML da Interface
HTML_PAGE = """
<!DOCTYPE html> 
<html lang="pt-br">
<head>
    <meta charset="UTF-8">
    <title>Compilador José - Web IDE</title>
    <style>
        body {{ font-family: 'Segoe UI', sans-serif; background-color: #f4f4f9; padding: 20px; margin: 0;}}
        .container {{ display: flex; gap: 20px; max-width: 1400px; margin: 0 auto; height: 90vh; }}
        .box {{ flex: 1; display: flex; flex-direction: column; }}
        textarea {{ padding: 10px; font-family: 'Consolas', monospace; font-size: 14px; border: 1px solid #ccc; border-radius: 5px; resize: none; background-color: #fff; }}
        #codigo-fonte {{ flex: 1; }}
        #output-mvd {{ flex: 1; background-color: #e9ecef; color: #495057; }}
        #output-console {{ flex: 1; background-color: #1e1e1e; color: #00ff00; line-height: 1.4; }}
        .header-box {{ display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px; }}
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
                    <button type="submit" class="btn-submit">Compilar e Executar</button>
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
            <h2>Console (Saída da Execução)</h2>
            <textarea id="output-console" readonly>{resultado}</textarea>
        </div>
    </div>
    </form>
    <script>
        document.getElementById('clear-btn').addEventListener('click', function() {{
            document.getElementById('codigo-fonte').value = '';
            document.getElementById('output-mvd').value = '';
            document.getElementById('output-console').value = 'Pronto para compilar...';
            document.getElementById('download-area').innerHTML = '';
        }});
    </script>
</body>
</html>
"""

DOWNLOAD_FILENAME = "programa.obj"

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
        self.wfile.write(HTML_PAGE.format(
            codigo_anterior="", 
            resultado="Pronto para compilar...",
            codigo_mvd="",
            vm_input="",
            download_link="").encode('utf-8'))

    def do_POST(self):
        logging.info("Requisição POST recebida para compilar e executar.")
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length).decode('utf-8')
        parsed_data = urllib.parse.parse_qs(post_data)
        codigo_fonte = parsed_data.get('codigo', [''])[0]
        vm_input = parsed_data.get('vm_input', [''])[0]

        # Salva o código fonte
        logging.info(f"Salvando código fonte em '{ARQUIVO_ENTRADA}'.")
        with open(ARQUIVO_ENTRADA, "w", encoding="utf-8") as f:
            f.write(codigo_fonte)

        resultado_texto = ""
        codigo_mvd_texto = ""
        download_link_html = ""

        try:
            # 1. Executa o COMPILADOR
            comando_compilador = [EXECUTAVEL_COMPILADOR, ARQUIVO_ENTRADA]
            logging.info(f"Executando compilador: {' '.join(comando_compilador)}")
            processo_compilacao = subprocess.run(
                comando_compilador,
                capture_output=True, text=True
            )
            logging.info(f"Compilador finalizado com código de saída: {processo_compilacao.returncode}")
            
            # Mostra logs do compilador
            if processo_compilacao.stdout:
                resultado_texto += "--- COMPILAÇÃO ---\n" + processo_compilacao.stdout + "\n"
            if processo_compilacao.stderr:
                resultado_texto += "--- ERROS DE COMPILAÇÃO ---\n" + processo_compilacao.stderr + "\n"

            # 2. Se compilou com sucesso, executa a MÁQUINA VIRTUAL
            if processo_compilacao.returncode == 0 and os.path.exists(ARQUIVO_SAIDA_MVD):
                logging.info("Compilação bem-sucedida. Preparando para executar a VM.")

                # Lê o código MVD gerado para exibição
                with open(ARQUIVO_SAIDA_MVD, 'r', encoding='utf-8') as f_mvd:
                    codigo_mvd_texto = f_mvd.read()
                
                download_link_html = f'<a href="/{ARQUIVO_SAIDA_MVD}" download><button type="button" class="btn-download">Baixar .obj</button></a>'
                
                try:
                    # Executa a VM passando o arquivo .mvd gerado
                    # IMPORTANTE: input='' impede que a VM trave esperando input se não houver 'leia'
                    # Mas se tiver 'leia', na Web isso é complexo. 
                    # A VM agora recebe o input do campo 'vm_input' da interface (ainda não implementado no HTML, mas a lógica está aqui)
                    # Por enquanto, assumimos execução não-interativa ou entrada fixa.
                    # Para permitir input interativo na web, seria necessário WebSocket, o que complica muito.
                    # Solução paliativa: A VM vai rodar, mas 'leia' vai pegar EOF ou erro na web simples.
                    comando_vm = [EXECUTAVEL_VM, ARQUIVO_SAIDA_MVD]
                    logging.info(f"Executando VM: {' '.join(comando_vm)}")
                    processo_vm = subprocess.run(
                        comando_vm,
                        capture_output=True, text=True, input=vm_input
                    )
                    logging.info(f"VM finalizada com código de saída: {processo_vm.returncode}")
                    
                    resultado_texto += "\n=========================\n"
                    resultado_texto += "    EXECUÇÃO (VM)    \n"
                    resultado_texto += "=========================\n"
                    resultado_texto += processo_vm.stdout
                    
                    if processo_vm.stderr:
                        resultado_texto += "\n[Erro na Execução]:\n" + processo_vm.stderr

                except Exception as e_vm:
                    logging.error(f"Erro ao executar a VM: {e_vm}")
                    resultado_texto += f"\nErro ao rodar VM: {e_vm}"
            
            else:
                logging.warning("Falha na compilação. A execução da VM foi abortada.")
                resultado_texto += "\n[Compilação Falhou - Execução abortada]"

        except Exception as e:
            logging.critical(f"Erro crítico no manipulador POST: {e}")
            resultado_texto = f"Erro crítico no servidor: {e}"

        self.send_response(200)
        self.send_header("Content-type", "text/html; charset=utf-8")
        self.end_headers()
        self.wfile.write(HTML_PAGE.format(
            codigo_anterior=codigo_fonte, 
            resultado=resultado_texto, 
            codigo_mvd=codigo_mvd_texto,
            download_link=download_link_html
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