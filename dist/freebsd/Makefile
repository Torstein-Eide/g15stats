PORTNAME=	g15stats
PORTVERSION=	3.0.3
CATEGORIES=	sysutils
MASTER_SITES=   https://gitlab.com/menelkir/${PORTNAME}/-/archive/${PORTVERSION}/

MAINTAINER=	dmenelkir@gmail.com
COMMENT=	Resource monitor for G15daemon

LICENSE=	GPLv2

LIB_DEPENDS+=	libg15.so:sysutils/libg15 \
		libg15render.so:sysutils/libg15render \
		libgtop-2.0.so:devel/libgtop \
		libg15daemon_client.so:sysutils/g15daemon

USES=		gmake localbase libtool gnome pkgconfig

USE_LDCONFIG=	yes

USE_GNOME+=	glib20

GNU_CONFIGURE=	yes

CFLAGS+=	-std=c99 -fcommon

USE_RC_SUBR=	g15stats

INSTALL_TARGET=	install-strip

PLIST_FILES=	bin/g15stats \
		etc/rc.d/g15stats

.include <bsd.port.mk>
