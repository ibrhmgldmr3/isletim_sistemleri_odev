# 
#  İşletim Sistemleri Proje Ödevi - 2024 Güz
#  Grup Üyeleri:
#  - Süleyman Samet Kaya - B221210103 - suleyman.kaya3@ogr.sakarya.edu.tr
#  - İbrahim Güldemir - B221210052 - ibrahim.guldemir@ogr.sakarya.edu.tr
#  - İsmail Konak - G221210046 - ismail.konak1@ogr.sakarya.edu.tr
#  - Tuğra Yavuz - B221210064 - tugra.yavuz@ogr.sakarya.edu.tr
#  - İsmail Alper Karadeniz - B221210065 - alper.karadeniz@ogr.sakarya.edu.tr
#  
#  Dosya: kabuk.c
#  Açıklama: Bu dosya, basit bir Linux kabuğunun C dilinde implementasyonunu içerir.
# 
program: derle main

derle:
	gcc -I ./include/ -o ./lib/shell.o -c ./src/shell.c
	gcc -o ./bin/Main ./lib/shell.o


main:
	./bin/Main
