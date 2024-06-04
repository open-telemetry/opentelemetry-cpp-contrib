const { logs, upAll, stop } = require('docker-compose');
const assert = require('assert');
const path = require('path');
const Tail = require('tail').Tail;
const axios = require('axios');
const fs = require('fs').promises;

const host = 'http://localhost:8000';
const traceFile = path.join(__dirname, '../data/trace.json');

async function waitForStatup(name, startupCheck) {
    return new Promise((resolve, reject) => {
        logs(name, { follow: true, cwd: path.join(__dirname, '../'), callback: (chunk) => {
            const line = chunk.toString('utf8');
            if (startupCheck(line)) {
                resolve();
            }
        }})
        .catch(reject);
    });
}

async function startServicesAndWait(services) {

    // Make sure collector is able to write to trace.json
    await fs.chmod(traceFile, 0666);

    return new Promise((resolve, reject) => {
        upAll({
            cwd: path.join(__dirname, '../'),
            // log: true,
        })
        .then(async () => {
            
            await Promise.all(
                services
                    .filter(service => service.startupCheck !== undefined)
                    .map(service => waitForStatup(service.name, service.startupCheck))
            );

            resolve();
        })
        .catch(reject);
    });
}

async function waitNginx() {
    return new Promise(async (resolve) => {
        while (true) {
            try {
                console.log(`Waiting for nginx to be up...`);
                await axios.get(`${host}/up`);
                resolve();
                break;
            } catch (ex) {
                await new Promise((resolve) => setTimeout(resolve, 1000));
            }
        }
    });
}

async function getTrace(traceStream, timeout ) {
    return new Promise((resolve, reject) => {
        
        const listener = (line) => {
            try {
                const traceLine = JSON.parse(line);
                resolve(traceLine);
            } catch (ex) {
                reject(ex);
            }
        }

        traceStream.once('line', listener);

        if (timeout) {
            setTimeout(() => { 
                traceStream.removeListener('line', listener);
                reject(new Error('timeout'));
            }, timeout);
        }
    });
}

function attribValue(attribute) {
    if (!attribute || !attribute.value) return undefined;
    if (attribute.value.stringValue) return attribute.value.stringValue;
    if (attribute.value.intValue) return parseInt(attribute.value.intValue);
    if (attribute.value.arrayValue) return attribute.value.arrayValue.values.map((value) => attribValue({ value }));
    else return undefined;
}

function validateResourceAttributes(attributes, expectedAttributes) {
    expectedAttributes.forEach((expectedAttribute) => {
        
        const value = attribValue(attributes.find((attrib) => attrib.key === expectedAttribute.key));

        if (expectedAttribute.fn) {
            assert(expectedAttribute.fn(value));
        } else if (expectedAttribute.regex) {
            assert.match(value, expectedAttribute.regex);
        } else {
            if (Array.isArray(expectedAttribute.value)) {
                assert.deepEqual(value, expectedAttribute.value);   
            } else {
                assert.equal(value, expectedAttribute.value);
            }
        }
    });
}

const commonExpectedAttributes = [
    // { key: 'service.name', value: 'nginx' },
    { key: 'telemetry.sdk.name', value: 'opentelemetry' },
    { key: 'telemetry.sdk.language', value: 'cpp' },
    { key: 'telemetry.sdk.version', value: '1.8.1' },
];

describe('instrumentation-test', function () {

    traceStream = null;

    before('setup', async function () {
        this.timeout(30000);
        const services = [{
            name: 'collector',
            startupCheck: (line) => line.includes('Everything is ready.')
        }, {
            name: 'node-backend',
            startupCheck: (line) => line.includes('simple_express ready')
        }, {
            name: 'nginx',
        }, {
            name: 'php-backend',
        }];

        await startServicesAndWait(services);

        // Wait Nginx
        await waitNginx();

        // Open stream to ../data/trace.json
        traceStream = new Tail(traceFile);
        return;
    });

    after('cleanup', async function () {
        this.timeout(30000);
        traceStream.unwatch();
        await stop({ cwd: path.join(__dirname, '../') }).then(() => console.log('Stopped'));
    });

    it('HTTP upstream | resource attributes', async function () {

        this.timeout(5000);

        const res = await axios.get(`${host}/?foo=bar&x=42`);
        assert.equal(res.status, 200);

        const trace = await getTrace(traceStream);

        assert.equal(trace.resourceSpans.length, 2);

        assert.equal(trace.resourceSpans[0].instrumentationLibrarySpans.length, 1);
        assert.equal(trace.resourceSpans[0].instrumentationLibrarySpans[0].spans.length, 1);
        assert.equal(trace.resourceSpans[0].instrumentationLibrarySpans[0].spans[0].kind, "SPAN_KIND_CLIENT");

        assert.equal(trace.resourceSpans[1].instrumentationLibrarySpans.length, 1);
        assert.equal(trace.resourceSpans[1].instrumentationLibrarySpans[0].spans.length, 1);
        assert.equal(trace.resourceSpans[1].instrumentationLibrarySpans[0].spans[0].kind, "SPAN_KIND_SERVER");

        trace.resourceSpans.forEach((resourceSpan) => {
            validateResourceAttributes(resourceSpan.resource.attributes, [
                ...commonExpectedAttributes,
                { key: 'service.name', value: 'nginx-proxy' }
            ]);
        });
    })

    it("HTTP upstream | span attributes", async function () {

        const res = await axios.get(`${host}/?foo=bar&x=42`, { 
            headers: {
                "User-Agent": "otel-test",
                "My-Header": "My-Value"
            }
        });

        assert.equal(res.status, 200);

        const trace = await getTrace(traceStream);

        assert.equal(trace.resourceSpans.length, 2);

        trace.resourceSpans.forEach((resourceSpan) => {

            assert(resourceSpan.instrumentationLibrarySpans.length, 1);
            assert(resourceSpan.instrumentationLibrarySpans[0].spans.length, 1);

            validateResourceAttributes(resourceSpan.instrumentationLibrarySpans[0].spans[0].attributes, [
                { key: 'net.peer.ip', regex: /\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}/ },
                { key: 'net.peer.port', fn: (actual) => actual > 0 },
                { key: 'http.method', value: 'GET' },
                { key: 'http.flavor', value: '1.1' },
                { key: 'http.target', value: '/?foo=bar&x=42' },
                { key: 'http.host', value: 'localhost:8000' },
                { key: 'http.server_name', value: 'otel_test' },
                { key: 'http.scheme', value: 'http' },
                { key: 'http.status_code', value: 200 },
                { key: 'http.user_agent', value: 'otel-test' },
                { key: 'http.request.header.host', value: undefined },
                { key: 'http.request.header.user_agent', value: undefined },
                { key: 'http.request.header.my_header', value: undefined },
            ]);
        });

        assert.equal(trace.resourceSpans[0].instrumentationLibrarySpans[0].spans[0].kind, "SPAN_KIND_CLIENT");
        assert.equal(trace.resourceSpans[0].instrumentationLibrarySpans[0].spans[0].name, "simple_backend");
        assert.equal(trace.resourceSpans[1].instrumentationLibrarySpans[0].spans[0].kind, "SPAN_KIND_SERVER");
        assert.equal(trace.resourceSpans[1].instrumentationLibrarySpans[0].spans[0].name, "simple_backend");
    })

    it("location with opentelemetry_capture_headers on should capture headers", async function() {

        const res = await axios.get(`${host}/capture_headers`, { 
            headers: {
                "Request-Header": "Request-Value"
            }
        });

        assert.equal(res.status, 200);

        const trace = await getTrace(traceStream);

        assert.equal(trace.resourceSpans.length, 2);

        // Check server span only
        const resourceSpan = trace.resourceSpans[1];

        assert(resourceSpan.instrumentationLibrarySpans.length, 1);
        assert(resourceSpan.instrumentationLibrarySpans[0].spans.length, 1);

        validateResourceAttributes(resourceSpan.instrumentationLibrarySpans[0].spans[0].attributes, [
            { key: 'http.request.header.host', value: undefined },
            { key: 'http.request.header.user_agent', value: undefined },
            { key: 'http.request.header.request_header', value: ['Request-Value'] },
            { key: 'http.response.header.response_header', value: ['Response-Value'] },
        ]);
    });

    
    it("location with opentelemetry_capture_headers and sensitive header name should redact header value", async function () {

        const res = await axios.get(`${host}/capture_headers_with_sensitive_header_name`, { 
            headers: {
                "Request-Header": "Foo",
                "Request-Token": "Token",
            }
        });

        assert.equal(res.status, 200);

        const trace = await getTrace(traceStream);

        // Check server span only
        const resourceSpan = trace.resourceSpans[1];

        assert(resourceSpan.instrumentationLibrarySpans.length, 1);
        assert(resourceSpan.instrumentationLibrarySpans[0].spans.length, 1);

        validateResourceAttributes(resourceSpan.instrumentationLibrarySpans[0].spans[0].attributes, [
            { key: 'http.request.header.host', value: undefined },
            { key: 'http.request.header.user_agent', value: undefined },
            { key: 'http.request.header.request_header', value: ['[REDACTED]'] },
            { key: 'http.response.header.response_header', value: ['[REDACTED]'] },
            { key: 'http.request.header.request_token', value: ['[REDACTED]'] },
            { key: 'http.response.header.response_token', value: ['[REDACTED]'] },
        ]);
    });

    it("location with opentelemetry_capture_headers and sensitive header value should redact header value", async function () {

        const res = await axios.get(`${host}/capture_headers_with_sensitive_header_value`, {
            headers: {
                "Bar": "Request-Value"
            }
        });

        assert.equal(res.status, 200);

        const trace = await getTrace(traceStream);

        // Check server span only
        const resourceSpan = trace.resourceSpans[1];

        assert(resourceSpan.instrumentationLibrarySpans.length, 1);
        assert(resourceSpan.instrumentationLibrarySpans[0].spans.length, 1);

        validateResourceAttributes(resourceSpan.instrumentationLibrarySpans[0].spans[0].attributes, [
            { key: 'http.request.header.host', value: undefined },
            { key: 'http.request.header.user_agent', value: undefined },
            { key: 'http.request.header.bar', value: ['[REDACTED]'] },
            { key: 'http.response.header.bar', value: ['[REDACTED]'] },
        ]);
    });

    it("location without operation name should use operation name from server", async function () {

        const res = await axios.get(`${host}/no_operation_name`);

        assert.equal(res.status, 200);

        const trace = await getTrace(traceStream);

        assert.equal(trace.resourceSpans.length, 2);

        trace.resourceSpans.forEach((resourceSpan) => {

            assert(resourceSpan.instrumentationLibrarySpans.length, 1);
            assert(resourceSpan.instrumentationLibrarySpans[0].spans.length, 1);

            assert.equal(resourceSpan.instrumentationLibrarySpans[0].spans[0].name, "otel_test");

        });
    });

    it("HTTP upstream | span is created when no traceparent exists", async function() {
        
        const res = await axios.get(`${host}`);

        assert.equal(res.status, 200);

        const trace = await getTrace(traceStream);

        assert.equal(trace.resourceSpans.length, 2);

        trace.resourceSpans.forEach((resourceSpan, idx) => {
            
            assert(resourceSpan.instrumentationLibrarySpans.length, 1);
            assert(resourceSpan.instrumentationLibrarySpans[0].spans.length, 1);

            validateResourceAttributes(resourceSpan.instrumentationLibrarySpans[0].spans[0].attributes, [
                { key: 'http.status_code', value: 200 },
            ]);

            assert.equal(resourceSpan.instrumentationLibrarySpans[0].spans[0].kind, idx === 0 ? "SPAN_KIND_CLIENT" : "SPAN_KIND_SERVER");
            if (idx === 1) {
                assert.equal(resourceSpan.instrumentationLibrarySpans[0].spans[0].parentSpanId, "");
            }
        });
    });

    it("HTTP upstream | span is associated with parent", async function() {
        
        const parentSpanId = "2a9d49c3e3b7c461"
        const inputTraceId = "aad85b4f655feed4d594a01cfa6a1d62"
    
        const res = await axios.get(host, {
            headers: {
                "traceparent": `00-${inputTraceId}-${parentSpanId}-00`
            }
        });
    
        assert.equal(res.status, 200);
    
        const parsedBody = res.data;
    
        const [prefix, traceId, spanId, suffix] = parsedBody.traceparent.split('-');
    
        assert.equal(prefix, '00');
        assert.equal(traceId, inputTraceId);
        assert.notEqual(spanId, parentSpanId);
        assert.equal(suffix, '01');
    
        const traces = await getTrace(traceStream);
    
        assert.equal(traces.resourceSpans.length, 2);
        
        const serverResourceSpan = traces.resourceSpans[1];
    
        assert(serverResourceSpan.instrumentationLibrarySpans.length, 1);
        assert(serverResourceSpan.instrumentationLibrarySpans[0].spans.length, 1);
    
        assert.equal(serverResourceSpan.instrumentationLibrarySpans[0].spans[0].traceId, inputTraceId);
        assert.equal(serverResourceSpan.instrumentationLibrarySpans[0].spans[0].parentSpanId, parentSpanId);
    
    });

    it("PHP-FPM upstream | span attributes", async function() {

        const res = await axios.get(`${host}/app.php`);

        assert.equal(res.status, 200);

        const trace = await getTrace(traceStream);

        assert.equal(trace.resourceSpans.length, 2);
        
        trace.resourceSpans.forEach((resourceSpan, idx) => {

            assert(resourceSpan.instrumentationLibrarySpans.length, 1);
            assert(resourceSpan.instrumentationLibrarySpans[0].spans.length, 1);

            validateResourceAttributes(resourceSpan.instrumentationLibrarySpans[0].spans[0].attributes, [
                { key: 'net.peer.ip', regex: /\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}/ },
                { key: 'net.peer.port', fn: (actual) => actual > 0 },
                { key: 'http.method', value: 'GET' },
                { key: 'http.flavor', value: '1.1' },
                { key: 'http.target', value: '/app.php' },
                { key: 'http.host', value: 'localhost:8000' },
                { key: 'http.server_name', value: 'otel_test' },
                { key: 'http.scheme', value: 'http' },
                { key: 'http.status_code', value: 200 },
            ]);

            assert.equal(resourceSpan.instrumentationLibrarySpans[0].spans[0].kind, idx === 0 ? "SPAN_KIND_CLIENT" : "SPAN_KIND_SERVER");
            assert.equal(resourceSpan.instrumentationLibrarySpans[0].spans[0].name, "php_fpm_backend");

        });
    });

    it("PHP-FPM upstream | span is associated with parent", async function() {

        const parentSpanId = "2a9d49c3e3b7c461"
        const inputTraceId = "aad85b4f655feed4d594a01cfa6a1d62"
    
        const res = await axios.get(`${host}/app.php`, {
            headers: {
                "traceparent": `00-${inputTraceId}-${parentSpanId}-00`
            }
        });
    
        assert.equal(res.status, 200);
    
        const parsedBody = res.data;
    
        const [prefix, traceId, spanId, suffix] = parsedBody.traceparent.split('-');
    
        assert.equal(prefix, '00');
        assert.equal(traceId, inputTraceId);
        assert.notEqual(spanId, parentSpanId);
        assert.equal(suffix, '01');
    
        const traces = await getTrace(traceStream);
    
        assert.equal(traces.resourceSpans.length, 2);
        
        const serverResourceSpan = traces.resourceSpans[1];
    
        assert(serverResourceSpan.instrumentationLibrarySpans.length, 1);
        assert(serverResourceSpan.instrumentationLibrarySpans[0].spans.length, 1);
    
        assert.equal(serverResourceSpan.instrumentationLibrarySpans[0].spans[0].traceId, inputTraceId);
        assert.equal(serverResourceSpan.instrumentationLibrarySpans[0].spans[0].parentSpanId, parentSpanId);
    });

    it("HTTP upstream | test b3 injection", async function() {

        const res = await axios.get(`${host}/b3`);

        assert.equal(res.status, 200);

        const trace = await getTrace(traceStream);

        assert.equal(trace.resourceSpans.length, 2);

        trace.resourceSpans.forEach((resourceSpan, idx) => {
            assert(resourceSpan.instrumentationLibrarySpans.length, 1);
            assert(resourceSpan.instrumentationLibrarySpans[0].spans.length, 1);

            assert.equal(resourceSpan.instrumentationLibrarySpans[0].spans[0].name, "test_b3");
            if (idx === 1) {
                assert.equal(resourceSpan.instrumentationLibrarySpans[0].spans[0].parentSpanId, "");
            }
        });
    });

    it("PHP-FPM upstream | test b3 injection", async function() {

        const res = await axios.get(`${host}/b3`);

        assert.equal(res.status, 200);

        const trace = await getTrace(traceStream);

        assert.equal(trace.resourceSpans.length, 2);

        trace.resourceSpans.forEach((resourceSpan, idx) => {
            assert(resourceSpan.instrumentationLibrarySpans.length, 1);
            assert(resourceSpan.instrumentationLibrarySpans[0].spans.length, 1);

            assert.equal(resourceSpan.instrumentationLibrarySpans[0].spans[0].name, "test_b3");
            if (idx === 1) {
                assert.equal(resourceSpan.instrumentationLibrarySpans[0].spans[0].parentSpanId, "");
            }
        });
    });

    it("HTTP upstream | test b3 propagation", async function() {

        const parentSpanId = "2a9d49c3e3b7c461"
        const inputTraceId = "aad85b4f655feed4d594a01cfa6a1d62"

        const res = await axios.get(`${host}/b3`, {
            headers: {
                "b3": `${inputTraceId}-${parentSpanId}-1`
            }
        });

        assert.equal(res.status, 200);

        const parsedBody = res.data;
        const [traceId, spanId] = parsedBody.b3.split('-');

        assert.equal(traceId, inputTraceId);
        assert.notEqual(spanId, parentSpanId);

        const traces = await getTrace(traceStream);

        assert.equal(traces.resourceSpans.length, 2);

        traces.resourceSpans.forEach((resourceSpan) => {
            assert(resourceSpan.instrumentationLibrarySpans.length, 1);
            assert(resourceSpan.instrumentationLibrarySpans[0].spans.length, 1);

            assert.equal(resourceSpan.instrumentationLibrarySpans[0].spans[0].traceId, inputTraceId);
            assert.notEqual(resourceSpan.instrumentationLibrarySpans[0].spans[0].spanId, parentSpanId);
            assert.equal(resourceSpan.instrumentationLibrarySpans[0].spans[0].name, "test_b3");
        });
    });

    it("HTTP upstream | multiheader b3 propagation", async function() {

        const parentSpanId = "2a9d49c3e3b7c461"
        const inputTraceId = "aad85b4f655feed4d594a01cfa6a1d62"

        const res = await axios.get(`${host}/b3`, {
            headers: {
                "X-B3-TraceId": inputTraceId,
                "X-B3-SpanId": parentSpanId,
                "X-B3-Sampled": "1"
            }
        });

        assert.equal(res.status, 200);

        const parsedBody = res.data;
        const [traceId, spanId] = parsedBody.b3.split('-');

        assert.equal(traceId, inputTraceId);
        assert.notEqual(spanId, parentSpanId);

        const traces = await getTrace(traceStream);

        assert.equal(traces.resourceSpans.length, 2);

        traces.resourceSpans.forEach((resourceSpan) => {
            assert(resourceSpan.instrumentationLibrarySpans.length, 1);
            assert(resourceSpan.instrumentationLibrarySpans[0].spans.length, 1);

            assert.equal(resourceSpan.instrumentationLibrarySpans[0].spans[0].traceId, inputTraceId);
            assert.notEqual(resourceSpan.instrumentationLibrarySpans[0].spans[0].spanId, parentSpanId);
            assert.equal(resourceSpan.instrumentationLibrarySpans[0].spans[0].name, "test_b3");
        });
    });

    it("Accessing a file produces a span", async function() {

        const res = await axios.get(`${host}/files/content.txt`);

        assert.equal(res.status, 200);

        const trace = await getTrace(traceStream);

        assert.equal(res.data, "Lorem Ipsum");

        assert.equal(trace.resourceSpans.length, 2);

        trace.resourceSpans.forEach((resourceSpan, idx) => {
            assert(resourceSpan.instrumentationLibrarySpans.length, 1);
            assert(resourceSpan.instrumentationLibrarySpans[0].spans.length, 1);

            assert.equal(resourceSpan.instrumentationLibrarySpans[0].spans[0].name, "file_access");
            assert.equal(resourceSpan.instrumentationLibrarySpans[0].spans[0].kind, idx === 0 ? "SPAN_KIND_INTERNAL" : "SPAN_KIND_SERVER");

            validateResourceAttributes(resourceSpan.instrumentationLibrarySpans[0].spans[0].attributes, [
                { key: 'http.method', value: 'GET' },
                { key: 'http.flavor', value: '1.1' },
                { key: 'http.target', value: '/files/content.txt' },
                { key: 'http.host', value: 'localhost:8000' },
                { key: 'http.server_name', value: 'otel_test' },
                { key: 'http.scheme', value: 'http' },
                { key: 'http.status_code', value: 200 },
            ]);
        });
    });

    it("Accessing a excluded uri produces no span", async function() {

        const res = await axios.get(`${host}/ignored.php`);

        assert.equal(res.status, 200);

        try {
            await getTrace(traceStream, 1000);
            assert.fail('expected timeout');
        } catch(ex) {
            assert.equal(ex.message, 'timeout');
        }
    });

    it("Accessing a route with disabled OpenTelemetry does not produce spans nor propagate", async function() {

        const res = await axios.get(`${host}/off`);

        assert.equal(res.status, 200);

        try {
            await getTrace(traceStream, 1000);
            assert.fail('expected timeout');
        } catch (ex) {
            assert.equal(ex.message, 'timeout');
        }
    });

    it("Accessing a route with disabled trustIncomingsSpans is not associated with a parent", async function() {

        const inputTraceId = "aad85b4f655feed4d594a01cfa6a1d62"

        const res = await axios.get(`${host}/distrust_incoming_spans`, {
            headers: {
                "traceparent": `00-${inputTraceId}-2a9d49c3e3b7c461-00`
            }
        });

        assert.equal(res.status, 200);

        const trace = await getTrace(traceStream);

        assert.equal(trace.resourceSpans.length, 2);
        
        const serverResourceSpan = trace.resourceSpans[1];
        assert.equal(serverResourceSpan.instrumentationLibrarySpans.length, 1);
        assert.equal(serverResourceSpan.instrumentationLibrarySpans[0].spans.length, 1);

        assert.notEqual(serverResourceSpan.instrumentationLibrarySpans[0].spans[0].traceId, inputTraceId);
        assert.equal(serverResourceSpan.instrumentationLibrarySpans[0].spans[0].parentSpanId, "");
    });

    it("Spans with custom attributes are produced", async function() {

        const res = await axios.get(`${host}/attrib`);

        assert.equal(res.status, 200);

        const trace = await getTrace(traceStream);

        assert.equal(trace.resourceSpans.length, 2);
        
        const serverResourceSpan = trace.resourceSpans[1];
        assert.equal(serverResourceSpan.instrumentationLibrarySpans.length, 1);
        assert.equal(serverResourceSpan.instrumentationLibrarySpans[0].spans.length, 1);

        validateResourceAttributes(serverResourceSpan.instrumentationLibrarySpans[0].spans[0].attributes, [
            { key: 'test.attrib.custom', value: 'local' },
            { key: 'test.attrib.global', value: 'global' },
            { key: 'test.attrib.script', regex: /\d+\.\d+/ },
        ]);
    });

    it("Accessing trace context", async function() {

        const res = await axios.get(`${host}/context`);

        assert.equal(res.status, 200);

        const traces = await getTrace(traceStream);

        assert.equal(traces.resourceSpans.length, 2);

        const innerResourceSpan = traces.resourceSpans[0];

        assert.equal(innerResourceSpan.instrumentationLibrarySpans.length, 1);
        assert.equal(innerResourceSpan.instrumentationLibrarySpans[0].spans.length, 1);

        const span = innerResourceSpan.instrumentationLibrarySpans[0].spans[0];
        const traceId = span.traceId;
        const spanId = span.spanId;

        const traceparent = res.headers['context-traceparent'];
        assert(traceparent);

        assert.equal(traceparent, `00-${traceId}-${spanId}-01`);
    });

    it("Accessing trace id", async function() {

        const res = await axios.get(`${host}/trace_id`);

        assert.equal(res.status, 200);

        const trace = await getTrace(traceStream);

        assert.equal(trace.resourceSpans.length, 2);

        const serverResourceSpan = trace.resourceSpans[1];
        const span = serverResourceSpan.instrumentationLibrarySpans[0].spans[0];

        const traceId = span.traceId;

        const traceIdHeader = res.headers['trace-id'];
        assert(traceIdHeader);

        assert.equal(traceIdHeader, traceId);
    });

    it("Accessing span id", async function() {

        const res = await axios.get(`${host}/span_id`);

        assert.equal(res.status, 200);

        const trace = await getTrace(traceStream);

        assert.equal(trace.resourceSpans.length, 2);

        const serverResourceSpan = trace.resourceSpans[1];

        assert.equal(serverResourceSpan.instrumentationLibrarySpans.length, 1);
        assert.equal(serverResourceSpan.instrumentationLibrarySpans[0].spans.length, 1);

        const span = serverResourceSpan.instrumentationLibrarySpans[0].spans[0];

        const spanId = span.spanId;

        const spanIdHeader = res.headers['span-id'];
        assert(spanIdHeader);

        assert.equal(spanIdHeader, spanId);
    });

    it("Accessing 301 redirect does not crash", async function () {

        const res = await axios.get(`${host}/redirect_301`, {
            maxRedirects: 0,
            validateStatus: (status) => {
                return status === 301;
            }
        });

        assert.equal(res.status, 301);

        const headers = res.headers;
        assert.equal(headers['location'], `${host}/redirect_301/`);

        await getTrace(traceStream);
    });

    it("Accessing internal request does not crash", async function () {

        const res = await axios.get(`${host}/route_to_internal`);

        assert.equal(res.status, 200);

        const headers = res.headers;
        const traceId = headers['trace-id'];

        assert(traceId);

        const traces = await getTrace(traceStream);

        assert.equal(traces.resourceSpans.length, 2);

        traces.resourceSpans.forEach((resourceSpan) => {
            assert(resourceSpan.instrumentationLibrarySpans.length, 1);
            assert(resourceSpan.instrumentationLibrarySpans[0].spans.length, 1);

            assert.equal(resourceSpan.instrumentationLibrarySpans[0].spans[0].traceId, traceId);
        });
    });

});
