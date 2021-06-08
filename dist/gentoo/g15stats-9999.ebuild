# Copyright 2021 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=7

inherit autotools git-r3

DESCRIPTION="CPU, memory, swap, network stats for G15 Keyboard"
HOMEPAGE="https://gitlab.com/menelkir/${PN}"
EGIT_REPO_URI="https://gitlab.com/menelkir/${PN}.git"

LICENSE="GPL-2"
SLOT="0"
IUSE=""
KEYWORDS=""

RDEPEND="app-misc/g15daemon
	dev-libs/libg15
	dev-libs/libg15render
	sys-libs/zlib
	gnome-base/libgtop"

DEPEND="${RDEPEND}"
BDEPEND="virtual/pkgconfig"

src_prepare() {
	default
	mv configure.{in,ac} || die
	eautoreconf
}

src_configure() {
	export CPPFLAGS="${CFLAGS}"
	econf
}

src_install() {
	default
	rm "${D}"/usr/share/doc/${PF}/{COPYING,NEWS} || die

	newconfd "${S}/contrib/init/${PN}.confd" ${PN}
	newinitd "${S}/contrib/init/${PN}.initd" ${PN}
}

pkg_postinst() {
	elog "Remember to set the interface you want monitored in"
	elog "/etc/conf.d/g15stats"
}
