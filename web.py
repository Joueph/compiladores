import http.server
import socketserver
import subprocess
import urllib.parse
import os


# CONFIGURAÇÕES
PORTA = 8000
EXECUTAVEL = "./analisador"
ARQUIVO_ENTRADA = "teste.txt"

# HTML da Interface (Note as chaves duplas {{ }} no CSS para evitar o erro)
HTML_PAGE = """
<!DOCTYPE html> 
<html lang="pt-br">
<head>
    <meta charset="UTF-8">
    <title>Compilador José - Web IDE</title>
    <style>
        body {{ font-family: sans-serif; background-color: #f4f4f9; padding: 20px; margin: 0;}}
        .container {{ display: flex; gap: 20px; max-width: 1200px; margin: 0 auto; height: 90vh; }}
        .box {{ flex: 1; display: flex; flex-direction: column; }}
        textarea {{ flex: 1; padding: 10px; font-family: monospace; font-size: 14px; border: 1px solid #ccc; border-radius: 5px; resize: none; }}
        #output {{ background-color: #1e1e1e; color: #00ff00; }}
        button {{ padding: 15px; background-color: #28a745; color: white; border: none; font-size: 16px; cursor: pointer; border-radius: 5px; margin-bottom: 10px; }}
        button:hover {{ background-color: #218838; }}
        h2 {{ margin-top: 0; }}
        #download-area {{ padding-top: 10px; }}
    </style>
</head>
<body>
    <div class="container">
        <div class="box">
            <h2>Código Fonte</h2>
            <form action="/" method="POST" id="formCompiler" style="display:contents;">
                <button type="submit">COMPILAR >> </button>
                <textarea name="codigo" placeholder="Digite seu código aqui...">{codigo_anterior}</textarea>
            </form>
        </div>
        <div class="box">
            <h2>Saída (Terminal / MVD)</h2>
            <div id="download-area">{download_link}</div>
            <textarea id="output" readonly>{resultado}</textarea>
        </div>
    </div>
</body>
</html>
"""
OBJ_FILENAME = "programa.mvd"
DOWNLOAD_FILENAME = "programa.obj"

class CompilerHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        # Rota para baixar o arquivo .mvd
        if self.path == f"/{OBJ_FILENAME}":
            if os.path.exists(OBJ_FILENAME):
                self.send_response(200)
                self.send_header("Content-type", "application/octet-stream")
                self.send_header("Content-Disposition", f'attachment; filename="{DOWNLOAD_FILENAME}"')
                self.end_headers()
                with open(OBJ_FILENAME, 'rb') as f:
                    self.wfile.write(f.read())
            else:
                self.send_response(404)
                self.send_header("Content-type", "text/html; charset=utf-8")
                self.end_headers()
                self.wfile.write(b"Arquivo nao encontrado. Compile o codigo primeiro.")
            return

        # Rota principal que exibe a página
        self.send_response(200)
        self.send_header("Content-type", "text/html; charset=utf-8")
        self.end_headers()
        # Renderiza a página inicial (chaves simples {} aqui são substituídas)
        self.wfile.write(HTML_PAGE.format(
            codigo_anterior="", 
            resultado="Aguardando compilacao...", 
            download_link="").encode('utf-8'))

    def do_POST(self):
        # 1. Ler o tamanho do conteúdo enviado
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length).decode('utf-8')
        
        # 2. Extrair o código do formulário
        parsed_data = urllib.parse.parse_qs(post_data)
        codigo_fonte = parsed_data.get('codigo', [''])[0]

        # 3. Salvar no arquivo teste.txt
        with open(ARQUIVO_ENTRADA, "w", encoding="utf-8") as f:
            f.write(codigo_fonte)

        # 4. Rodar o compilador C
        resultado_texto = ""
        download_link_html = ""
        try:
            processo = subprocess.run(
                [EXECUTAVEL, ARQUIVO_ENTRADA],
                capture_output=True,
                text=True
            )
            
            # Captura erros e saída padrão
            if processo.stderr:
                resultado_texto += "=== ERROS ===\n" + processo.stderr + "\n"
            if processo.stdout:
                resultado_texto += "=== LOG ===\n" + processo.stdout + "\n"
            
            if processo.returncode == 0:
                # Se deu certo, tenta ler o arquivo MVD gerado
                if os.path.exists(OBJ_FILENAME):
                    resultado_texto += f"\n=== SUCESSO ({OBJ_FILENAME}) ===\n"
                    with open(OBJ_FILENAME, "r", encoding="utf-8") as f:
                        resultado_texto += f.read()
                    download_link_html = f'<a href="/{OBJ_FILENAME}" download><button>Baixar {DOWNLOAD_FILENAME}</button></a>'
            else:
                resultado_texto += f"\n[Compilação falhou com código {processo.returncode}]"

        except Exception as e:
            resultado_texto = f"Erro ao executar o compilador: {e}\nVerifique se '{EXECUTAVEL}' existe e tem permissão de execução."

        # 5. Devolver a página com os resultados preenchidos
        self.send_response(200)
        self.send_header("Content-type", "text/html; charset=utf-8")
        self.end_headers()
        self.wfile.write(HTML_PAGE.format(
            codigo_anterior=codigo_fonte, 
            resultado=resultado_texto, 
            download_link=download_link_html
        ).encode('utf-8'))

if __name__ == "__main__":
    print(f"--- Servidor Iniciado ---")
    print(f"Acesse no navegador: http://localhost:{PORTA}")
    print(f"Para parar, pressione Ctrl+C no terminal.")
    
    server = socketserver.TCPServer(("0.0.0.0", PORTA), CompilerHandler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nServidor parado.")
        server.server_close()

        