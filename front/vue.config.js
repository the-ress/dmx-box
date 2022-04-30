module.exports = {
  productionSourceMap: false,
  devServer: {
    proxy: {
      '/api': {
        target: 'http://dmx-box:80',
        changeOrigin: true,
        ws: true
      }
    }
  }
}
