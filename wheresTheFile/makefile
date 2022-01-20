CC = gcc # set compiler
CFLAGS = -g #-Wall # add debugging to executable, turn on all compiler warnings
#VPATH = ./WTFDir:./WTFserverDir:./WTFshared

.PHONY: all # in case there's a file called 'all'
all: WTFtest WTFserver WTF

WTFtest: WTFtest.o errorCheck.o
	$(CC) $(CFLAGS) -o ./Server/WTFtest WTFtest.o errorCheck.o -pthread

WTFtest.o: WTFtest.c errorCheck.h
	$(CC) $(CFLAGS) -c WTFtest.c

WTF: WTF.o errorCheck.o fileNode.o directoryFunctions.o readSpecialFile.o getFromSocket.o sendToSocket.o clientDoCommit.o clientDoPush.o clientDoUpgrade.o
	$(CC) $(CFLAGS) -o ./Client/WTF WTF.o errorCheck.o fileNode.o directoryFunctions.o readSpecialFile.o getFromSocket.o sendToSocket.o clientDoCommit.o clientDoPush.o clientDoUpgrade.o -lssl -lcrypto -lm

WTF.o: WTF.c errorCheck.h fileNode.h directoryFunctions.h readSpecialFile.h getFromSocket.h sendToSocket.h clientDoCommit.h clientDoPush.h clientDoUpgrade.h
	$(CC) $(CFLAGS) -c WTF.c

clientDoCommit.o: clientDoCommit.c clientDoCommit.h errorCheck.h fileNode.h directoryFunctions.h readSpecialFile.h getFromSocket.h sendToSocket.h
	$(CC) $(CFLAGS) -c clientDoCommit.c

clientDoPush.o: clientDoPush.c clientDoPush.h errorCheck.h fileNode.h directoryFunctions.h readSpecialFile.h getFromSocket.h sendToSocket.h
	$(CC) $(CFLAGS) -c clientDoPush.c

clientDoUpgrade.o: clientDoUpgrade.c clientDoUpgrade.h errorCheck.h fileNode.h directoryFunctions.h readSpecialFile.h getFromSocket.h sendToSocket.h
	$(CC) $(CFLAGS) -c clientDoUpgrade.c

WTFserver: WTFserver.o serverRequestHandler.o getFromSocket.o errorCheck.o doCreate.o sendToSocket.o fileNode.o directoryFunctions.o doCheckout.o readSpecialFile.o doUpdate.o doUpgrade.o mutexHandler.o doRollback.o doDestroy.o doCommit.o doHistory.o doPush.o
	$(CC) $(CFLAGS) -o ./Server/WTFserver WTFserver.o serverRequestHandler.o getFromSocket.o errorCheck.o doCreate.o sendToSocket.o fileNode.o directoryFunctions.o doCheckout.o readSpecialFile.o doUpdate.o doUpgrade.o mutexHandler.o doRollback.o doDestroy.o doCommit.o doHistory.o doPush.o -pthread -lm

WTFserver.o: WTFserver.c serverRequestHandler.h errorCheck.h mutexHandler.h
	$(CC) $(CFLAGS) -c WTFserver.c

serverRequestHandler.o: serverRequestHandler.c serverRequestHandler.h getFromSocket.h doCreate.h doUpdate.h doUpgrade.h doRollback.h doDestroy.h doCommit.h doHistory.h doPush.h
	$(CC) $(CFLAGS) -c serverRequestHandler.c

getFromSocket.o: getFromSocket.c getFromSocket.h initGetFromSocket.h
	$(CC) $(CFLAGS) -c getFromSocket.c

errorCheck.o: errorCheck.c errorCheck.h
	$(CC) $(CFLAGS) -c errorCheck.c

doCreate.o: doCreate.c doCreate.h sendToSocket.h fileNode.h directoryFunctions.h mutexHandler.h
	$(CC) $(CFLAGS) -c doCreate.c

sendToSocket.o: sendToSocket.c sendToSocket.h fileNode.h
	$(CC) $(CFLAGS) -c sendToSocket.c

fileNode.o: fileNode.c fileNode.h
	$(CC) $(CFLAGS) -c fileNode.c

directoryFunctions.o: directoryFunctions.c directoryFunctions.h fileNode.h
	$(CC) $(CFLAGS) -c directoryFunctions.c

doCheckout.o: doCheckout.c doCheckout.h getFromSocket.h sendToSocket.h directoryFunctions.h readSpecialFile.h mutexHandler.h
	$(CC) $(CFLAGS) -c doCheckout.c

readSpecialFile.o: readSpecialFile.c readSpecialFile.h fileNode.h initGetFromSocket.h
	$(CC) $(CFLAGS) -c readSpecialFile.c

doUpdate.o: doUpdate.c doUpdate.h getFromSocket.h sendToSocket.h directoryFunctions.h mutexHandler.h
	$(CC) $(CFLAGS) -c doUpdate.c

doUpgrade.o: doUpgrade.c doUpgrade.h getFromSocket.h sendToSocket.h directoryFunctions.h initGetFromSocket.h mutexHandler.h
	$(CC) $(CFLAGS) -c doUpgrade.c

mutexHandler.o: mutexHandler.c mutexHandler.h
	$(CC) $(CFLAGS) -c mutexHandler.c

doRollback.o: doRollback.c doRollback.h initGetFromSocket.h getFromSocket.h directoryFunctions.h mutexHandler.h
	$(CC) $(CFLAGS) -c doRollback.c

doDestroy.o: doDestroy.c doDestroy.h getFromSocket.h directoryFunctions.h mutexHandler.h
	$(CC) $(CFLAGS) -c doDestroy.c

doCommit.o: doCommit.c doCommit.h getFromSocket.h sendToSocket.h directoryFunctions.h fileNode.h mutexHandler.h
	$(CC) $(CFLAGS) -c doCommit.c

doHistory.o: doHistory.c doHistory.h getFromSocket.h sendToSocket.h directoryFunctions.h fileNode.h mutexHandler.h
	$(CC) $(CFLAGS) -c doHistory.c

doPush.o: doPush.c doPush.h getFromSocket.h sendToSocket.h readSpecialFile.h directoryFunctions.h fileNode.h mutexHandler.h
	$(CC) $(CFLAGS) -c doPush.c

clean:
	rm ./Server/WTFtest WTFtest.o ./Server/WTFserver WTFserver.o serverRequestHandler.o  getFromSocket.o errorCheck.o doCreate.o sendToSocket.o fileNode.o directoryFunctions.o doCheckout.o readSpecialFile.o doUpdate.o doUpgrade.o mutexHandler.o doRollback.o doDestroy.o doCommit.o doHistory.o doPush.o ./Client/WTF WTF.o clientDoCommit.o clientDoPush.o clientDoUpgrade.o