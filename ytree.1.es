.\" extracted from ytree.doc 26 November 1996 by cmt2doc.pl
.TH YTREE 1 "19 de Febrero 1999" " " "PROGRAMAS"
.PD .8v
.SH NOMBRE
.nf
ytree \- Administrador de archivos \-
.fi

.SH SINOPSIS

ytree [\fInómbre de archivo \f | \fIdirectorio\fR]

.SH DESCRIPCION

Si no hay argumentos de línea de comandos, será usado el directorio actual.

Son provistos de los siguientes comandos

1.) Ventana de DRIECTORIOS:
.TP
\fB\-Attribute\fR
Cambiar permisos de directorio (como chmod)
.TP
\fB\-Delete\fR
Borrar el directorio seleccionado
.TP
\fB\-Filespec\fR
Configurar máscara (exp. regulares)
e.g. *.[ch] para todos los ficheros *.c y *.h \-
.TP
\fB\-Group\fR
Cambiar el grupo dueño del directorio
.TP
\fB\-Log\fR
Reiniciar ytree con un nuevo directorio/archivo raiz 
.TP
\fB\-Makedir\fR
Crear un nuevo directorio
.TP
\fB\-Owner\fR
Cambiar el usuario dueño del directorio/fichero seleccionado
.TP
\fB\-Rename\fR
Renombrar el directorio o fichero seleccionado
.TP
\fB\-Showall\fR
Mostrar todos los ficheros de todos los directorios cargados en memoria 
.TP
\fB\-Tag\fR
Marcar todos los ficheros en el directorio seleccionado
.TP
\fB\-Untag\fR
Desmarcar todos los ficheros del directorio seleccionado
.TP
\fB\-eXecute\fR
Ejecutar un shell con un comando
.TP
\fB\-^Filemode\fR
Cambiar el modo de vista:
.nf
.RS
\fB\- Solamente nómbres\fR
\fB\- Nómbre, atributos, links, tamaño,\fR
\fB  fecha de modificación, link simbólico\fR
\fB\- nómbre, atributos, inode, dueño, grupo, link simbólico\fR
\fB\- change status-, fecha de acceso, link simbólico\fR
.fi
.RE
.TP
\fB\-Return\fR
Cambiar a la siguiente ventana
.PP
2.) Ventana de FICHEROS
.TP
\fB\-Attribute\fR
Cambiar los permisos del fichero (como chmod)
.TP
\fB\-^Attribute\fR
Cambiar los permisos de todos los ficheros marcados.
\fB?\fR significa: no cambiar el atributo
.TP
\fB\-Copy\fR
Copiar el fichero
.TP
\fB\-^K Copy\fR
Copiar todos los ficheros marcados
.TP
\fB\-Delete\fR
Borrar el fichero seleccionado
.TP
\fB\-^Delete\fR
Borrar todos los ficheros seleccionados
.TP
\fB\-Edit\fR
Editar el fichero con  $EDITOR (ver ~/.ytree)
o \- si no esta ninguno definido \- vi
.TP
\fB\-Filespec\fR
Configurar mascara (expresiones regulares)
e.g. *.[ch] para todos los *.c y *.h \- Files
.TP
\fB\-Group\fR
Cambiar el grupo del fichero seleccionado
.TP
\fB\-^Group\fR
Cambiar el grupo de todos los ficheros marcados
.TP
\fB\-Hex\fR
Ver el fichero seleccionado con HEXDUMP (ver ~/.ytree),
o \- si no esta definido \- hd / od \-h
.TP
\fB\-Log\fR
Reiniciar ytree con un nuevo directorio/archivo como raiz
.TP
\fB\-Move\fR
mover el fichero seleccionado
.TP
\fB\-^N Move\fR
Mover todos los ficheros marcados
.TP
\fB\-Owner\fR
Cambiar el usuario dueño del fichero seleccionado
.TP
\fB\-^Owner\fR
Cambiar el usuario dueño de todos los ficheros marcados
.TP
\fB\-Pipe\fR
Enviar el contenido del archivo a traves de un comando
.TP
\fB\-^Pipe\fR
Enviar el contenido de todos los ficheros a traves de un comando
.TP
\fB\-Rename\fR
Renombrar el fichero marcado
.TP
\fB\-^Rename\fR
Renombrar todos los ficheros marcados
.TP
\fB\-Sort\fR
Ordenar la lista de ficheros por
.nf
.RS
\fB\- Fecha de acceso\fR
\fB\- Fecha de cambio\fR
\fB\- Extensión\fR
\fB\- Grupo\fR
\fB\- Fecha de modificación\fR
\fB\- Nómbre\fR
\fB\- Dueño\fR
\fB\- Tamaño\fR
.fi
.RE
.TP
\fB\-Tag\fR
Marcar el fichero seleccionado
.TP
\fB\-^Tag\fR
Marcar todos los ficheros mostrados
.TP
\fB\-Untag\fR
Desmarcar el fichero seleccionado
.TP
\fB\-^Untag\fR
Desmarcar todos los ficheros mostrados
.TP
\fB\-View\fR
Ver el fichero con el paginador definido en ~/.ytree
o - si no esta definido - con pg \-cen
.TP
\fB\-eXecute\fR
Ejecutar un shell con un comando
.TP
\fB\-e^Xecute\fR
Ejecutar un shell con un comando para todos los ficheros marcados.
El texto {} es reemplazado por el fichero actual
.TP
\fB\-pathcopY\fR
Copiar el fichero selecionado con el camino
.TP
\fB\-pathcop^Y\fR
Copiar todos los ficheros marcados con el camino
.TP
\fB\-^Filemode\fR
Cambiar el modo de vista para los ficheros:
.nf
.RS
\fB\- solamente nómbres\fR
\fB\- nómbre, atributos, links, tamaño, fecha de modificación,\fR
\fB  link simbólico\fR
\fB\- nómbre, atributos, inode, dueño, grupo, link simbólico\fR
\fB\- changestatus-, fecha de acceso, link simbólico\fR
.fi
.RE
.TP
\fB\-^L\fR
Redibujar la terminal
.TP
\fB\-Space\fR
Suprimir la salida por terminal mientras esta trabajando
.TP
\fB\-Return\fR
Cambiar a la ventana expandida
.PP
3.) Ventana de directorios
.TP
\fB\-Filespec\fR
Configurar máscara(expresiones regulares)
e.g. *.[ch] para todos ficheros los *.c y *.h \-
.TP
\fB\-Log\fR
Reiniciar ytree con un nuevo directorio/archivo como raiz
.TP
\fB\-Showall\fR
Mostrar todos los ficheros de todos los directorios
.TP
\fB\-Tag\fR
Marcar todos los ficheros en el directorio seleccionado
.TP
\fB\-Untag\fR
Desmarcar todos los ficheros en el directorio seleccionado
.TP
\fB\-eXecute\fR
Ejecutar un shell con un comando
.TP
\fB\-^Filemode\fR
Cambiar el modo de vista para los ficheros:
.nf
.RS
\fB\- Solamente nómbres de ficheros\fR
\fB\- nómbres, atributos, links, tamaño, fecha de modificación\fR
\fB\- nómbre, atributos, dueño, grupo\fR
.fi
.RE
.TP
\fB\-^L\fR
Redibujar la terminal
.PP
4.) Ventana de Archivos:
.TP
\fB\-Copy\fR
Copiar el fichero seleccionado
.TP
\fB\-^K Copy\fR
Copiar todos los ficheros marcados
.TP
\fB\-Filespec\fR
Configurar la mascara (expresiones regulares)
e.g. *.[ch] para todos los ficheros *.c y *.h \-
.TP
\fB\-Hex\fR
Ver el fichero seleccionado con HEXDUMP (ver ~/.ytree),
o \- si no esta definido \- hd / od \-h
.TP
\fB\-Pipe\fR
Pipe content of all tagged to a command
Enviar el contenido de todos los ficheros marcados a traves de un comando
.TP
\fB\-Sort\fR
Ordenar la lista de ficheros por
.nf
.RS
\fB\- Fecha de acceso\fR
\fB\- Fecha de cambio\fR
\fB\- Extensión\fR
\fB\- Grupo\fR
\fB\- Fecha de modificación\fR
\fB\- Nómbre\fR
\fB\- Dueño\fR
\fB\- Tamaño\fR
.fi
.RE
.TP
\fB\-Tag\fR
Marcar el fichero seleccionado
.TP
\fB\-^Tag\fR
Marcar todos los ficheros en el directorio seleccionado
.TP
\fB\-Untag\fR
Desmarcar todos los ficheros del directorio seleccionado
.TP
\fB\-View\fR
Ver el fichero con el paginador definido en ~/.ytree
o - si no esta definido - con pg \-cen
.TP
\fB\-eXecute\fR
Ejecutar un shell con un comando
.TP
\fB\-^Filemode\fR
Cambiar el modo de vista para los ficheros:
.nf
.RS
\fB\- solamente nómbres\fR
\fB\- nómbres, atributos, links, tamaño\fR
.fi
.RE
.TP
\fB\-^L\fR
Redibujar la terminal
.TP
\fB\-Return\fR
Cambiar a ventana expandida
.PP
ytree cambia a ventana de archivo automáticamentes simplemente
por seleccionar un fichero con el commando \fILog\fR o por
ejecutar ytree desde la línea de comandos con un fichero como
argumento.

ytree reconoce la extensión del  fichero ".F", ".Z", ".z" y
".gz" y ejecuta el descompresor apropiado MELT 
(por defecto=melt), UNCOMPRESS (por defecto=uncompress) y GNUUNZIP
(por defecto=gunzip \-c). Usted puede cambiar estas configuraciones
en ~/.ytree.

ytree reconoce los siguientes tipos de archivos y requiere:
.nf
.RS
.sp 1
\fB  archivos TAR: gtar     (revisado con GNU-TAR 1.11.2)\fR
\fB  archivos ARC: arc      (revisado con arc 5.12 02/05/86)\fR
\fB  archivos LHA: xlharc   (revisado con xlharc V1.02 1989)\fR
\fB  archivos ZIP: unzip    (revisado con unzip v5.0 08/92)\fR
\fB  "         " : zipinfo  (revisado con zipinfo v1.0 08/92)\fR
\fB  archivos ZOO: zoo      (revisado con zoo v2.10)\fR
.fi
.RE

Los archivos son reconocidos por la extension del nombre(e.g zoo, zip).
Los archivos TAR pueden tener las siguientes extensiones:
.nf 
.RS
.sp 1
\fB  ".F", ".TFR", ".Faa",                            (freeze)\fR
\fB  ".Z", ".TZ", ".TZR", ".Xaa",                   (compress)\fR
\fB  ".z", ".gz", ".tzr", ".tz", ".xaa", ".tgz", ".TGZ",\fR
\fB  ".taz", ".TAZ", ".tpz" und ".TPZ"                  (gzip)\fR
.fi
.RE


El comando VER es configurable en la seccion [VIEWER] de ~/.ytree:

Example:

.nf 
\fB [VIEWER]
\fB .jpg,.gif,.bmp,.tif,.ppm,.xpm=xv
\fB .1,.2,.3,.4,.5,.6,.7,.8,.n=nroff -man | less
\fB .ps=ghostview
\fB .mid,.MID=playmidi -e
\fB .wav,.WAV=splay
\fB .au=auplay
\fB .avi,.mpg,.mov=xanim
\fB .htm,.html=lynx
\fB .pdf,.PDF=acroread
\fB .mp3=mpg123
.fi
.PP 

Un historial de lineas es soportado: Use las teclas up/down.
Use "F2" en la línea de comando para seleccionar directorios.

.SH FICHEROS

$HOME/.ytree	fichero de configuracion del ytree


.SH ERRORES

Para evitar  problemas con las sequencias de escape en máquinas RS/6000 
(telnet/rlogin) configure la variable de entorno ESCDELAY:
.nf
.sp 1
ESCDELAY=1000
export ESCDELAY
.fi

.SH AUTOR

W. Bregulla (werner@frolix.han.de)

.SH Traduccion del manual

Carlos Barros (cbf@debian.org)
