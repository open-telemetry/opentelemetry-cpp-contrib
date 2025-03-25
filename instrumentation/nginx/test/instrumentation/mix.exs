defmodule Instrumentation.MixProject do
  use Mix.Project

  def project do
    [
      app: :instrumentation,
      version: "0.1.0",
      elixir: "~> 1.11",
      start_permanent: Mix.env() == :prod,
      deps: deps()
      # elixirc_paths: elixirc_paths(Mix.env())
    ]
  end

  # defp elixirc_paths(:test), do: ["lib", "test/helpers"]
  # defp elixirc_paths

  defp deps do
    [
      {:httpoison, "2.2.1"},
      {:jason, "1.4.4"}
    ]
  end
end
