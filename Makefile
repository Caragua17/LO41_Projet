
CXX = gcc
FLG = -pthread

run: compil

compil:
	$(CXX) Immeuble.c -o Immeuble $(FLG)
	$(CXX) Visiteur.c -o Visiteur $(FLG)
	$(CXX) Ascenseur.c -o Ascenseur $(FLG)
	$(CXX) Resident.c -o Resident $(FLG) 
