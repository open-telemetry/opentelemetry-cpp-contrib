defmodule Instrumentation.MixProject do
  use Mix.Project

  def project do
    [
      app: :instrumentation,
      version: "0.1.0",
      elixir: "~> 1.11",
      start_permanent: Mix.env() == :prod,
      deps: deps()
    ]
  end

  defp deps do
    [
      {:httpoison, "1.8.0"},
      {:jason, "1.2.2"}
    ]
  end
end
