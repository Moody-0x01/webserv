const base = () => document.getElementById('baseUrl').value.replace(/\/$/, '');
const url  = name => `${base()}/post/${encodeURIComponent(name)}`;

let passed = 0, failed = 0;

function log(msg, type='info') {
    const lb = document.getElementById('logBody');
    const t = new Date().toTimeString().slice(0,8);
    const el = document.createElement('div');
    el.className = 'log-line';
    el.innerHTML = `<span class="log-time">${t}</span><span class="log-${type}">${msg}</span>`;
    lb.appendChild(el);
    lb.scrollTop = lb.scrollHeight;
}

function clearLog() {
    document.getElementById('logBody').innerHTML = '';
    passed = 0; failed = 0; updateStats();
}

function updateStats() {
    document.getElementById('statPass').textContent  = passed;
    document.getElementById('statFail').textContent  = failed;
    document.getElementById('statTotal').textContent = passed + failed;
}

function setCard(id, state, label) {
    const card = document.getElementById(`test-${id}`);
    const st   = document.getElementById(`status-${id}`);
    const sp   = document.getElementById(`spin-${id}`);
    card.className = `test-card ${state}`;
    st.className   = `card-status ${state}`;
    st.textContent = label;
    if (state === 'running') {
      sp.classList.add('visible');
    } else {
      sp.classList.remove('visible');
      if (state === 'success') { passed++; updateStats(); }
      if (state === 'error')   { failed++; updateStats(); }
    }
}

function showResponse(id, status, body, ms) {
    const rh = document.getElementById(`rh-${id}`);
    const rb = document.getElementById(`rb-${id}`);
    const sc = document.getElementById(`sc-${id}`);
    const rt = document.getElementById(`rt-${id}`);
    rh.classList.add('visible');
    rb.classList.add('visible');
    rb.value = body;
    rt.textContent = ms ? `${ms}ms` : '';
    sc.textContent = status;
    sc.className = 'status-code';
    if (typeof status === 'number') {
      if (status >= 200 && status < 300) sc.classList.add('sc-2xx');
      else if (status >= 400 && status < 500) sc.classList.add('sc-4xx');
      else sc.classList.add('sc-5xx');
    } else {
      sc.classList.add('sc-err');
    }
}

async function req(method, filename, body, contentType) {
    const opts = { method, headers: {} };
    if (body !== undefined) {
      if (body instanceof FormData) {
        opts.body = body;
      } else {
        opts.headers['Content-Type'] = contentType || 'application/octet-stream';
        opts.body = body;
      }
    }
    const t0 = Date.now();
    const r = await fetch(url(filename), opts);
    const ms = Date.now() - t0;
    let text;
    try { text = await r.text(); } catch { text = '(no body)'; }
    return { status: r.status, ok: r.ok, body: text, ms };
}

function toggle(id) {
    const el = document.getElementById(id);
    el.style.display = el.style.display === 'none' ? '' : 'none';
}

function scrollTo(id) {
    document.getElementById(id)?.scrollIntoView({ behavior: 'smooth', block: 'start' });
}

// Drag & drop
function dzDrag(e, id) { e.preventDefault(); document.getElementById(id).classList.add('drag'); }
function dzLeave(id)    { document.getElementById(id).classList.remove('drag'); }
function dzDrop(e, inputId, dzId, fpId) {
    e.preventDefault();
    dzLeave(dzId);
    const file = e.dataTransfer.files[0];
    if (!file) return;
    const input = document.getElementById(inputId);
    const dt = new DataTransfer(); dt.items.add(file); input.files = dt.files;
    showPreviewFile(file, fpId, inputId.replace('-file','') + '-name');
}

function showPreview(inputId, fpId, nameId) {
    const file = document.getElementById(inputId).files[0];
    if (!file) return;
    showPreviewFile(file, fpId, nameId);
}

function showPreviewFile(file, fpId, nameId) {
    const fp = document.getElementById(fpId);
    fp.classList.add('visible');
    document.getElementById(fpId + '-name').textContent = `${file.name} (${(file.size/1024).toFixed(1)} KB)`;

    // Forcefully update the POST upload filename input
    const nameInput = document.getElementById(nameId);
    if (nameInput) nameInput.value = file.name;

    // Sync the GET filename input
    const getInput = document.getElementById('get-name');
    if (getInput) getInput.value = file.name;

    // Sync the DELETE filename input
    const deleteInput = document.getElementById('delete-name');
    if (deleteInput) deleteInput.value = file.name;
}

function clearFile(inputId, fpId) {
    document.getElementById(inputId).value = '';
    document.getElementById(fpId).classList.remove('visible');
}

// ── Ping ────────────────────────────────────────────────
async function pingServer() {
    const dot  = document.getElementById('statusDot');
    const txt  = document.getElementById('statusText');
    txt.textContent = 'checking…';
    try {
      const r = await fetch(base() + '/post/__ping_test__', { method: 'GET', signal: AbortSignal.timeout(3000) });
      dot.className = 'dot online';
      txt.textContent = ' online';
      log(`Server responded — ${base()}`, 'ok');
    } catch (e) {
      dot.className = 'dot offline';
      txt.textContent = ' offline';
      log(`Cannot reach ${base()} because ${e}`, 'fail');
    }
}

// ── Tests ───────────────────────────────────────────────
async function doUpload() {
    const name = document.getElementById('upload-name').value.trim();
    const fileInput = document.getElementById('upload-file');

    // Toggle this or change it to read from your radio buttons/UI state
    let isChunked = false; 

    if (!name) return alert('Enter a filename.');
    if (!fileInput.files.length) return alert('Please select a file to upload.');

    // Grab the actual file object
    const file = fileInput.files[0];
    const typeLabel = isChunked ? 'Chunked' : 'Content-Length';

    setCard('upload', 'running', 'sending…');
    log(`POST /post/${name} [${typeLabel}]`, 'info');

    try {
      const opts = {
        method: 'POST',
        headers: {
          // Use the file's type, not the input element's type
          'Content-Type': file.type || 'application/octet-stream'
        }
      };

      if (isChunked) {
        // Stream directly from the file object
        opts.body = file.stream();
        opts.duplex = 'half'; // CRITICAL: Required by browsers for chunked upload streams
      } else {
        // Read file into an ArrayBuffer for a fixed Content-Length body
        opts.body = await file.arrayBuffer();
      }

      const t0 = Date.now();
      // Bypassing the req() helper so fetch receives the duplex option correctly
      const r = await fetch(url(name), opts); 
      const ms = Date.now() - t0;
      
      let text;
      try { text = await r.text(); } catch { text = '(no body)'; }
      
      showResponse('upload', r.status, text, ms);
      if (r.ok) {
        setCard('upload', 'success', `${r.status} OK`);
        log(`POST ${name} [${typeLabel}] → ${r.status} (${ms}ms)`, 'ok');
      } else {
        setCard('upload', 'error', `${r.status}`);
        log(`POST ${name} [${typeLabel}] → ${r.status}`, 'fail');
      }
    } catch (e) {
      setCard('upload', 'error', 'network error');
      showResponse('upload', 'ERR', e.message);
      log(`POST ${name} → ${e.message}`, 'fail');
    }
}
async function doGet() {
    const name = document.getElementById('get-name').value.trim();
    if (!name) return alert('Enter a filename.');
    
    setCard('get', 'running', 'fetching…');
    log(`GET /post/${name}`, 'info');
    
    try {
      const t0 = Date.now();
      const r = await fetch(url(name), { method: 'GET' });
      const ms = Date.now() - t0;
      
      const contentType = r.headers.get('Content-Type') || '';
      const blob = await r.blob();
      
      // UI Elements
      const rh = document.getElementById(`rh-get`);
      const rb = document.getElementById(`rb-get`);
      const sc = document.getElementById(`sc-get`);
      const rt = document.getElementById(`rt-get`);
      const preview = document.getElementById('preview-get');
      
      // Setup Header
      rh.classList.add('visible');
      rt.textContent = `${ms}ms`;
      sc.textContent = r.status;
      sc.className = 'status-code ' + (r.ok ? 'sc-2xx' : (r.status < 500 ? 'sc-4xx' : 'sc-5xx'));
      
      // Reset displays
      rb.classList.remove('visible');
      preview.style.display = 'none';
      preview.innerHTML = '';

      if (r.ok) {
        setCard('get', 'success', `${r.status} OK`);
        log(`GET ${name} → ${r.status} (${ms}ms) [${contentType}]`, 'ok');
        
        const objectUrl = URL.createObjectURL(blob);
        
        // Handle Media Types dynamically
        if (contentType.startsWith('image/')) {
          preview.innerHTML = `<img src="${objectUrl}" style="max-width: 100%; max-height: 400px; border-radius: 4px;" />`;
          preview.style.display = 'block';
        } 
        else if (contentType.startsWith('video/')) {
          preview.innerHTML = `<video controls src="${objectUrl}" style="max-width: 100%; max-height: 400px; border-radius: 4px;"></video>`;
          preview.style.display = 'block';
        } 
        else if (contentType.startsWith('audio/')) {
          preview.innerHTML = `<audio controls src="${objectUrl}" style="width: 100%;"></audio>`;
          preview.style.display = 'block';
        } 
        else if (contentType.startsWith('text/') || contentType === 'application/json' || contentType === 'application/javascript') {
          // If it is text, read the blob and show it in the textarea
          rb.value = await blob.text();
          rb.classList.add('visible');
        } 
        else {
          // Fallback for unrenderable binaries (PDFs, zip files, octet-streams)
          preview.innerHTML = `<div style="color: var(--muted); font-size: 12px; font-family: var(--font-mono);">Media type <b>${contentType || 'unknown'}</b> downloaded. Cannot preview natively.</div>`;
          preview.style.display = 'block';
        }
      } else {
        // If 404 or other error, display the error text
        setCard('get', 'error', `${r.status}`);
        log(`GET ${name} → ${r.status}`, 'fail');
        rb.value = await blob.text();
        rb.classList.add('visible');
      }
    } catch (e) {
      setCard('get', 'error', 'network error');
      
      const sc = document.getElementById('sc-get');
      const rb = document.getElementById('rb-get');
      
      sc.textContent = 'ERR';
      sc.className = 'status-code sc-err';
      rb.value = e.message;
      rb.classList.add('visible');
      
      log(`GET ${name} → ${e.message}`, 'fail');
    }
}
async function doGetDownload() {
    const name = document.getElementById('get-name').value.trim();
    if (!name) return alert('Enter a filename.');
    try {
      const r = await fetch(url(name));
      const blob = await r.blob();
      const a = document.createElement('a');
      a.href = URL.createObjectURL(blob);
      a.download = name;
      a.click();
      log(`Downloaded ${name}`, 'ok');
    } catch (e) {
      log(`Download failed: ${e.message}`, 'fail');
    }
}

async function doDelete() {
    const name = document.getElementById('delete-name').value.trim();
    if (!name) return alert('Enter a filename.');
    setCard('delete', 'running', 'deleting…');
    log(`DELETE /post/${name}`, 'info');
    try {
      const r = await req('DELETE', name);
      showResponse('delete', r.status, r.body, r.ms);
      if (r.ok) {
        setCard('delete', 'success', `${r.status} OK`);
        log(`DELETE ${name} → ${r.status} (${r.ms}ms)`, 'ok');
      } else {
        setCard('delete', 'error', `${r.status}`);
        log(`DELETE ${name} → ${r.status}`, 'fail');
      }
    } catch (e) {
      setCard('delete', 'error', 'network error');
      showResponse('delete', 'ERR', e.message);
      log(`DELETE ${name} → ${e.message}`, 'fail');
    }
}


async function doGetMissing() {
    const name = document.getElementById('missing-name').value.trim();
    setCard('missing', 'running', 'testing…');
    log(`GET /post/${name} (expect 404)`, 'info');
    try {
      const r = await req('GET', name);
      showResponse('missing', r.status, r.body, r.ms);
      if (r.status === 404) {
        setCard('missing', 'success', '404 ✓');
        log(`GET ${name} → 404 as expected ✓`, 'ok');
      } else {
        setCard('missing', 'error', `unexpected ${r.status}`);
        log(`GET ${name} → ${r.status} (expected 404)`, 'fail');
      }
    } catch (e) {
      setCard('missing', 'error', 'network error');
      showResponse('missing', 'ERR', e.message);
      log(`GET missing → ${e.message}`, 'fail');
    }
}

async function doDeleteMissing() {
    const name = document.getElementById('del-missing-name').value.trim();
    setCard('del-missing', 'running', 'testing…');
    log(`DELETE /post/${name} (expect error)`, 'info');
    try {
      const r = await req('DELETE', name);
      showResponse('del-missing', r.status, r.body, r.ms);
      if (!r.ok) {
        setCard('del-missing', 'success', `${r.status} ✓`);
        log(`DELETE missing → ${r.status} ✓`, 'ok');
      } else {
        setCard('del-missing', 'error', `unexpected ${r.status}`);
        log(`DELETE missing → ${r.status} (expected error)`, 'fail');
      }
    } catch (e) {
      setCard('del-missing', 'error', 'network error');
      showResponse('del-missing', 'ERR', e.message);
      log(`DELETE missing → ${e.message}`, 'fail');
    }
}

// ── Run All ──────────────────────────────────────────────
async function runAll() {
    const fileInput = document.getElementById('upload-file');
    if (!fileInput.files.length) return alert('Please select a file to upload.');
    log('═══ Running all tests ═══', 'info');
    await doUpload();
    await new Promise(r => setTimeout(r, 400));
    await doGet();
    await new Promise(r => setTimeout(r, 400));
    await doGet();
    await new Promise(r => setTimeout(r, 400));
    await doDelete();
    await new Promise(r => setTimeout(r, 400));
    await doGetMissing();
    await new Promise(r => setTimeout(r, 400));
    await doDeleteMissing();
    log(`═══ Done — ${passed} passed, ${failed} failed ═══`, passed > 0 && failed === 0 ? 'ok' : 'fail');
}

// Auto-ping on load
window.addEventListener('load', pingServer);