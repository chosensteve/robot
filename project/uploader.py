from flask import Flask, request, render_template_string 
import os

UPLOAD_FOLDER = '/mnt/d-drive/phone backup/backup'
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

app = Flask(__name__)
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER

HTML = '''
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <title>NAS Uploader</title>
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <style>
    body {
      background-color: #121212;
      color: #e0e0e0;
      font-family: sans-serif;
      text-align: center;
      padding: 2rem;
    }
    h2 {
      margin-bottom: 2rem;
    }
    form {
      background: #1e1e1e;
      padding: 2rem;
      border: 2px dashed #444;
      border-radius: 1rem;
      max-width: 400px;
      margin: 0 auto;
    }
    input[type="file"],
    input[type="submit"] {
      display: block;
      width: 100%;
      margin: 1rem 0;
      padding: 1rem;
      font-size: 1.1rem;
      border: none;
      border-radius: 0.5rem;
    }
    input[type="submit"] {
      background: #03a9f4;
      color: white;
      cursor: pointer;
    }
  </style>
</head>
<body>
  <h2>Upload to NAS</h2>
  <form method="post" enctype="multipart/form-data">
    <input type="file" name="files" multiple required />
    <input type="submit" value="Upload" />
  </form>
</body>
</html>

'''
@app.route('/', methods=['GET', 'POST'])
def upload_file():
    if request.method == 'POST':
        uploaded_files = request.files.getlist('files')
        for file in uploaded_files:
            if file and file.filename:
                file.save(os.path.join(app.config['UPLOAD_FOLDER'], file.filename))
        return '''
            <!doctype html>
            <html>
              <head>
                <meta charset="utf-8">
                <meta name="viewport" content="width=device-width, initial-scale=1">
                <title>Upload Complete</title>
                <style>
                  body {
                    background-color: #121212;
                    color: #e0e0e0;
                    font-family: sans-serif;
                    text-align: center;
                    padding-top: 4rem;
                  }
                  a {
                    color: #03a9f4;
                    font-size: 1.2rem;
                    display: inline-block;
                    margin-top: 2rem;
                    text-decoration: underline;
                  }
                </style>
              </head>
              <body>
                <h1>Upload successful!</h1>
                <a href="/">Back to upload page</a>
              </body>
            </html>
        '''
    return render_template_string(HTML)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
