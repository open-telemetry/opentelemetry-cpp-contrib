defmodule TraceProto do
  defmodule SpanKind do
    def unspecified, do: 0
    def internal, do: 1
    def server, do: 2
    def client, do: 3
  end
end
