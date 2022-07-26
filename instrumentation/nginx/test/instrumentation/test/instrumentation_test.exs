defmodule InstrumentationTest do
  use ExUnit.Case

  @host "localhost:8000"
  @traces_path "../data/trace.json"

  def has_line(lines, re) do
    Enum.find(lines, fn line -> String.match?(line, re) end) != nil
  end

  def poll_nginx(0), do: raise("Timed out waiting for nginx")

  def poll_nginx(attempts_remaining) do
    case HTTPoison.get("#{@host}/up") do
      {:ok, %HTTPoison.Response{status_code: 200}} ->
        :ready

      _ ->
        Process.sleep(200)
        poll_nginx(attempts_remaining - 1)
    end
  end

  def wait_nginx() do
    poll_nginx(30)
  end

  def wait_until_ready(_, %{:collector => true, :express => true, :nginx => false}) do
    wait_nginx()
    :ready
  end

  def wait_until_ready(port, ctx) do
    receive do
      {_, {:data, output}} ->
        IO.puts(output)
        lines = String.split(output, "\n", trim: true)

        has_collector = ctx[:collector] || has_line(lines, ~r/everything is ready/i)
        has_express = ctx[:express] || has_line(lines, ~r/simple_express ready/i)

        wait_until_ready(
          port,
          Map.merge(ctx, %{
            collector: has_collector,
            express: has_express,
            nginx: false
          })
        )
    after
      30_000 -> raise "Timed out waiting for docker containers"
    end
  end

  def wait_until_ready(port) do
    wait_until_ready(port, %{})
  end

  def read_traces(_file, num_traces, _timeout, traces) when num_traces <= 0, do: traces

  def read_traces(_file, _num_traces, timeout, _traces) when timeout <= 0,
    do: raise("timed out waiting for traces")

  def read_traces(file, num_traces, timeout, traces) do
    case IO.read(file, :line) do
      :eof ->
        Process.sleep(100)
        read_traces(file, num_traces, timeout - 100, traces)

      line ->
        read_traces(file, num_traces - 1, timeout, [Jason.decode!(line) | traces])
    end
  end

  def read_traces(file, num_traces, timeout \\ 1_000) do
    read_traces(file, num_traces, timeout, [])
  end

  def collect_resource_spans(trace) do
    trace["resourceSpans"]
  end

  def collect_spans(trace) do
    [resource_spans] = collect_resource_spans(trace)
    [il_spans] = resource_spans["instrumentationLibrarySpans"]
    il_spans["spans"]
  end

  def values(map) do
    Enum.map(map, fn {k, v} ->
      case k do
        "intValue" ->
          String.to_integer(v)

        "arrayValue" ->
          [t] = Enum.map(v["values"], fn v -> values(v) end)
          t

        _ ->
          v
      end
    end)
  end

  def attrib(span, key) do
    case Enum.find(span["attributes"], fn %{"key" => k} -> k == key end) do
      %{"value" => val} ->
        [v] = values(val)
        v

      _ ->
        nil
    end
  end

  def read_until_eof(file, lines) do
    case IO.read(file, :line) do
      :eof ->
        lines

      line ->
        read_until_eof(file, [line | lines])
    end
  end

  def read_until_eof(file) do
    read_until_eof(file, [])
  end

  setup_all do
    File.chmod!(@traces_path, 0o666)
    port = Port.open({:spawn, "docker-compose up"}, [:binary])

    on_exit(fn -> System.cmd("docker-compose", ["down"]) end)

    wait_until_ready(port)

    trace_file = File.open!(@traces_path, [:read])

    on_exit(fn ->
      File.close(trace_file)
    end)

    %{trace_file: trace_file}
  end

  setup %{trace_file: trace_file} = ctx do
    read_until_eof(trace_file)
    ctx
  end

  test "HTTP upstream | resource attributes", %{trace_file: trace_file} do
    %HTTPoison.Response{status_code: status} = HTTPoison.get!("#{@host}/?foo=bar&x=42")

    [trace] = read_traces(trace_file, 1)
    [resource_spans] = collect_resource_spans(trace)
    resource = resource_spans["resource"]

    assert status == 200

    assert attrib(resource, "service.name") == "nginx-proxy"
  end

  test "HTTP upstream | span attributes", %{trace_file: trace_file} do
    %HTTPoison.Response{status_code: status} = HTTPoison.get!("#{@host}/?foo=bar&x=42", [{"User-Agent", "otel-test"}, {"My-Header", "My-Value"}])

    [trace] = read_traces(trace_file, 1)
    [span] = collect_spans(trace)

    assert status == 200

    assert attrib(span, "net.host.port") == 8000
    assert attrib(span, "net.peer.ip") =~ ~r/\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}/
    assert attrib(span, "net.peer.port") > 0

    assert attrib(span, "http.method") == "GET"
    assert attrib(span, "http.flavor") == "1.1"
    assert attrib(span, "http.target") == "/?foo=bar&x=42"
    assert attrib(span, "http.host") == @host
    assert attrib(span, "http.server_name") == "otel_test"
    assert attrib(span, "http.scheme") == "http"
    assert attrib(span, "http.status_code") == 200
    assert attrib(span, "http.user_agent") == "otel-test"
    assert attrib(span, "http.request.header.host") == nil
    assert attrib(span, "http.request.header.user_agent") == nil
    assert attrib(span, "http.request.header.my_header") == nil

    assert span["kind"] == "SPAN_KIND_SERVER"
    assert span["name"] == "simple_backend"
  end

  test "location with opentelemetry_capture_headers on should capture headers", %{trace_file: trace_file} do
    %HTTPoison.Response{status_code: status} = HTTPoison.get!("#{@host}/capture_headers", [{"Request-Header", "Request-Value"}])

    [trace] = read_traces(trace_file, 1)
    [span] = collect_spans(trace)

    assert status == 200

    assert attrib(span, "http.request.header.host") == nil
    assert attrib(span, "http.request.header.user_agent") == nil
    assert attrib(span, "http.request.header.request_header") == ["Request-Value"]
    assert attrib(span, "http.response.header.response_header") == ["Response-Value"]
  end

  test "location with opentelemetry_capture_headers and sensitive header name should redact header value", %{trace_file: trace_file} do
    %HTTPoison.Response{status_code: status} = HTTPoison.get!("#{@host}/capture_headers_with_sensitive_header_name", [{"Request-Header", "Foo"}])

    [trace] = read_traces(trace_file, 1)
    [span] = collect_spans(trace)

    assert status == 200

    assert attrib(span, "http.request.header.host") == nil
    assert attrib(span, "http.request.header.user_agent") == nil
    assert attrib(span, "http.request.header.request_header") == ["[REDACTED]"]
    assert attrib(span, "http.response.header.response_header") == ["[REDACTED]"]
  end

  test "location with opentelemetry_capture_headers and sensitive header value should redact header value", %{trace_file: trace_file} do
    %HTTPoison.Response{status_code: status} = HTTPoison.get!("#{@host}/capture_headers_with_sensitive_header_value", [{"Bar", "Request-Value"}])

    [trace] = read_traces(trace_file, 1)
    [span] = collect_spans(trace)

    assert status == 200

    assert attrib(span, "http.request.header.host") == nil
    assert attrib(span, "http.request.header.user_agent") == nil
    assert attrib(span, "http.request.header.bar") == ["[REDACTED]"]
    assert attrib(span, "http.response.header.bar") == ["[REDACTED]"]
  end

   test "location without operation name should use operation name from server", %{trace_file: trace_file} do
    %HTTPoison.Response{status_code: status} = HTTPoison.get!("#{@host}/no_operation_name")

    [trace] = read_traces(trace_file, 1)
    [span] = collect_spans(trace)

    assert status == 200
    assert span["name"] == "otel_test"
  end

  def test_parent_span(url, %{trace_file: trace_file}) do
    parent_span_id = "2a9d49c3e3b7c461"
    input_trace_id = "aad85b4f655feed4d594a01cfa6a1d62"

    %HTTPoison.Response{status_code: status, body: body} =
      HTTPoison.get!(url, [
        {"traceparent", "00-#{input_trace_id}-#{parent_span_id}-00"}
      ])

    %{"traceparent" => traceparent} = Jason.decode!(body)
    ["00", trace_id, span_id, "01"] = String.split(traceparent, "-")

    [trace] = read_traces(trace_file, 1)
    [span] = collect_spans(trace)

    assert status == 200
    assert trace_id == input_trace_id
    assert span_id != parent_span_id
    assert String.length(span_id) == 16

    assert span["parentSpanId"] == parent_span_id
    assert span["spanId"] != parent_span_id
  end

  test "HTTP upstream | span is created when no traceparent exists", %{trace_file: trace_file} do
    %HTTPoison.Response{status_code: status} = HTTPoison.get!(@host)

    [trace] = read_traces(trace_file, 1)
    [span] = collect_spans(trace)

    assert status == 200
    assert span["parentSpanId"] == ""
    assert attrib(span, "http.status_code") == 200
  end

  test "HTTP upstream | span is associated with parent", ctx do
    test_parent_span(@host, ctx)
  end

  test "PHP-FPM upstream | span attributes", %{trace_file: trace_file} do
    %HTTPoison.Response{status_code: status} = HTTPoison.get!("#{@host}/app.php")

    [trace] = read_traces(trace_file, 1)
    [span] = collect_spans(trace)

    assert status == 200
    assert attrib(span, "http.method") == "GET"
    assert attrib(span, "http.flavor") == "1.1"
    assert attrib(span, "http.target") == "/app.php"
    assert attrib(span, "http.host") == @host
    assert attrib(span, "http.server_name") == "otel_test"
    assert attrib(span, "http.scheme") == "http"
    assert attrib(span, "http.status_code") == 200

    assert span["parentSpanId"] == ""
    assert span["kind"] == "SPAN_KIND_SERVER"
    assert span["name"] == "php_fpm_backend"
  end

  test "PHP-FPM upstream | span is associated with parent", ctx do
    test_parent_span("#{@host}/app.php", ctx)
  end

  test "HTTP upstream | test b3 injection", %{trace_file: trace_file} do
    %HTTPoison.Response{status_code: status} = HTTPoison.get!("#{@host}/b3")

    [trace] = read_traces(trace_file, 1)
    [span] = collect_spans(trace)

    assert status == 200
    assert span["parentSpanId"] == ""
    assert span["name"] == "test_b3"
  end

  test "PHP-FPM upstream | test b3 injection", %{trace_file: trace_file} do
    %HTTPoison.Response{status_code: status} = HTTPoison.get!("#{@host}/b3")

    [trace] = read_traces(trace_file, 1)
    [span] = collect_spans(trace)

    assert status == 200
    assert span["parentSpanId"] == ""
    assert span["name"] == "test_b3"
  end

  test "HTTP upstream | test b3 propagation", %{trace_file: trace_file} do
    parent_span_id = "2a9d49c3e3b7c461"
    input_trace_id = "aad85b4f655feed4d594a01cfa6a1d62"

    %HTTPoison.Response{status_code: status, body: body} =
      HTTPoison.get!("#{@host}/b3", [
        {"b3", "#{input_trace_id}-#{parent_span_id}-1"}
      ])

    [trace] = read_traces(trace_file, 1)
    [span] = collect_spans(trace)

    %{"b3" => b3} = Jason.decode!(body)
    [trace_id, span_id, _] = String.split(b3, "-")
    assert trace_id == input_trace_id
    assert span_id != parent_span_id

    assert status == 200
    assert span["parentSpanId"] == parent_span_id
    assert span["spanId"] != parent_span_id
    assert span["name"] == "test_b3"
  end

  test "HTTP upstream | multiheader b3 propagation", %{trace_file: trace_file} do
    parent_span_id = "2a9d49c3e3b7c461"
    input_trace_id = "aad85b4f655feed4d594a01cfa6a1d62"

    %HTTPoison.Response{status_code: status, body: body} =
      HTTPoison.get!("#{@host}/b3", [
        {"X-B3-TraceId", input_trace_id},
        {"X-B3-SpanId", parent_span_id},
        {"X-B3-Sampled", "1"}
      ])

    [trace] = read_traces(trace_file, 1)
    [span] = collect_spans(trace)

    %{"b3" => b3} = Jason.decode!(body)
    [trace_id, span_id, _] = String.split(b3, "-")
    assert trace_id == input_trace_id
    assert span_id != parent_span_id

    assert status == 200
    assert span["parentSpanId"] == parent_span_id
    assert span["spanId"] != parent_span_id
    assert span["name"] == "test_b3"
  end

  test "Accessing a file produces a span", %{trace_file: trace_file} do
    %HTTPoison.Response{status_code: status, body: body} =
      HTTPoison.get!("#{@host}/files/content.txt")

    [trace] = read_traces(trace_file, 1)
    [span] = collect_spans(trace)

    assert body == "Lorem Ipsum"
    assert status == 200
    assert attrib(span, "http.method") == "GET"
    assert attrib(span, "http.flavor") == "1.1"
    assert attrib(span, "http.target") == "/files/content.txt"
    assert attrib(span, "http.host") == @host
    assert attrib(span, "http.server_name") == "otel_test"
    assert attrib(span, "http.scheme") == "http"
    assert attrib(span, "http.status_code") == 200

    assert span["parentSpanId"] == ""
    assert span["kind"] == "SPAN_KIND_SERVER"
    assert span["name"] == "file_access"
  end

  test "Accessing a excluded uri produces no span", %{trace_file: trace_file} do
    %HTTPoison.Response{status_code: status, body: body} =
      HTTPoison.get!("#{@host}/ignored.php")

    assert_raise RuntimeError, "timed out waiting for traces", fn ->
      read_traces(trace_file, 1)
    end

    assert status == 200
  end

  test "Accessing a route with disabled OpenTelemetry does not produce spans nor propagate", %{
    trace_file: trace_file
  } do
    %HTTPoison.Response{status_code: status} = HTTPoison.get!("#{@host}/off")

    assert_raise RuntimeError, "timed out waiting for traces", fn ->
      read_traces(trace_file, 1)
    end

    assert status == 200
  end

  test "Accessing a route with disabled trustIncomingsSpans is not associated with a parent", %{
    trace_file: trace_file
  } do
    input_trace_id = "aad85b4f655feed4d594a01cfa6a1d62"

    %HTTPoison.Response{status_code: status} =
      HTTPoison.get!("#{@host}/distrust_incoming_spans", [
        {"traceparent", "00-#{input_trace_id}-2a9d49c3e3b7c461-00"}
      ])

    [trace] = read_traces(trace_file, 1)
    [span] = collect_spans(trace)

    assert status == 200
    assert span["traceId"] != input_trace_id
    assert span["parentSpanId"] == ""
  end

  test "Spans with custom attributes are produced", %{
    trace_file: trace_file
  } do
    %HTTPoison.Response{status_code: status} = HTTPoison.get!("#{@host}/attrib")
    [trace] = read_traces(trace_file, 1)
    [span] = collect_spans(trace)
    assert attrib(span, "test.attrib.custom") == "local"
    assert attrib(span, "test.attrib.global") == "global"
    assert attrib(span, "test.attrib.script") =~ ~r/\d+\.\d+/
    assert status == 200
  end

  test "Accessing trace context", %{
    trace_file: trace_file
  } do
    %HTTPoison.Response{status_code: status, headers: headers} =
      HTTPoison.get!("#{@host}/context")

    [trace] = read_traces(trace_file, 1)
    [span] = collect_spans(trace)
    {_, header_context} = Enum.find(headers, fn {k, _} -> k == "Context-TraceParent" end)
    trace_id = span["traceId"]
    span_id = span["spanId"]
    context = "00-#{trace_id}-#{span_id}-01"
    assert header_context == context
    assert status == 200
  end

  test "Accessing trace id", %{
    trace_file: trace_file
  } do
    %HTTPoison.Response{status_code: status, headers: headers} =
      HTTPoison.get!("#{@host}/trace_id")

    [trace] = read_traces(trace_file, 1)
    [span] = collect_spans(trace)
    {_, header_trace_id} = Enum.find(headers, fn {k, _} -> k == "Trace-Id" end)
    assert header_trace_id == span["traceId"]
    assert status == 200
  end

  test "Accessing span id", %{
    trace_file: trace_file
  } do
    %HTTPoison.Response{status_code: status, headers: headers} =
      HTTPoison.get!("#{@host}/span_id")

    [trace] = read_traces(trace_file, 1)
    [span] = collect_spans(trace)
    {_, header_span_id} = Enum.find(headers, fn {k, _} -> k == "Span-Id" end)
    assert header_span_id == span["spanId"]
    assert status == 200
  end

  test "Accessing 301 redirect does not crash", %{
    trace_file: trace_file
  } do
    %HTTPoison.Response{status_code: status, headers: headers} =
      HTTPoison.get!(
        "#{@host}/redirect_301",
        [],
        follow_redirect: false
      )

    assert status == 301

    {_, header_location} = Enum.find(headers, fn {k, _} -> k == "Location" end)
    assert header_location == "http://#{@host}/redirect_301/"
  end

  test "Accessing internal request does not crash", %{
    trace_file: trace_file
  } do
    %HTTPoison.Response{status_code: status, headers: headers} =
      HTTPoison.get!("#{@host}/route_to_internal")

    [trace] = read_traces(trace_file, 1)
    [span] = collect_spans(trace)
    {_, header_trace_id} = Enum.find(headers, fn {k, _} -> k == "Trace-Id" end)
    assert header_trace_id == span["traceId"]
    assert status == 200
  end
end
