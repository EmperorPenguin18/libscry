# Maintainer: Sebastien MacDougall-Landry

pkgname=libscry
pkgver=0.3.1
pkgrel=1
pkgdesc='A Magic: The Gathering library'
url='https://github.com/EmperorPenguin18/libscry/'
source=("$pkgname-$pkgver.tar.gz::https://github.com/EmperorPenguin18/libscry/archive/refs/tags/$pkgver.tar.gz")
arch=('x86_64')
license=('GPL3')
depends=('curl' 'sqlite' 'rapidjson')
sha256sums=('fa4526f61650b38aab5dde76da234b068f8a86870dd159bce411d65b5e118ace')

build () {
  cd "$srcdir/$pkgname-$pkgver"
  g++ -O3 -std=c++20 -pthread -fPIC -shared src/*.cc -o libscry.so
}

package () {
  cd "$srcdir/$pkgname-$pkgver"
  install -Dm755 libscry.so -t "$pkgdir/usr/lib"
  install -Dm644 src/*.h -t "$pkgdir/usr/include"
}
