require_relative 'server'
Rack::Handler::Thin.run Sinatra::Application, :Port => 4567, :Host => '0.0.0.0'

# thin -R config.ru start