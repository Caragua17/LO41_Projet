
CXX = gcc

run: compil

compil:
	$(CXX) Immeuble.c -o Immeuble
	$(CXX) Visiteur.c -o Visiteur
	$(CXX) Ascenseur.c -o Ascenseur
	$(CXX) Resident.c -o Resident
