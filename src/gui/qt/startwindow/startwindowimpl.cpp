/***************************************************************************
 *   Copyright (C) 2006 by FThauer FHammer   *
 *   f.thauer@web.de   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "startwindowimpl.h"

#include "playerinterface.h"

#include <gamedata.h>
#include <generic/serverguiwrapper.h>
#include <net/socket_msg.h>
#include "tools.h"

#include "session.h"
#include "game.h"
#include "guiwrapper.h"

#include "configfile.h"
#include "gametableimpl.h"
#include "newgamedialogimpl.h"
#include "aboutpokerthimpl.h"
#include "mymessagedialogimpl.h"
#include "settingsdialogimpl.h"
#include "selectavatardialogimpl.h"
#include "joinnetworkgamedialogimpl.h"
#include "connecttoserverdialogimpl.h"
#include "createnetworkgamedialogimpl.h"
#include "startnetworkgamedialogimpl.h"
#include "changecontentdialogimpl.h"
#include "changecompleteblindsdialogimpl.h"
#include "gamelobbydialogimpl.h"
#include "timeoutmsgboximpl.h"
#include "chattools.h"
#include "serverlistdialogimpl.h"
#include "internetgamelogindialogimpl.h"

using namespace std;

startWindowImpl::startWindowImpl(ConfigFile *c)
	: myConfig(c), msgBoxOutdatedVersionActive(false)
{

	myGuiInterface.reset(new GuiWrapper(myConfig, this));
	{
		mySession.reset(new Session(myGuiInterface.get(), myConfig));
		mySession->init(); // TODO handle error
		// 		myGuiInterface->setSession(session);
	}

	// #ifdef __APPLE__
	// 	setWindowModality(Qt::ApplicationModal);
	// 	setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
	// #endif
	setupUi(this);
	this->setWindowTitle(QString(tr("PokerTH %1").arg(POKERTH_BETA_RELEASE_STRING)));
	this->installEventFilter(this);

	//Widgets Grafiken per Stylesheets setzen
	QString myAppDataPath = QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str());
	this->setWindowIcon(QIcon(myAppDataPath+"gfx/gui/misc/windowicon.png"));

	this->setStatusBar(0);

	centralwidget->setStyleSheet(".QWidget { background-image: url(\""+myAppDataPath+"gfx/gui/misc/startwindowbg.png\"); background-position: bottom center; background-origin: content; background-repeat: no-repeat;}");

	// 	Dialogs
	myNewGameDialog = new newGameDialogImpl(this, myConfig);
	mySelectAvatarDialog = new selectAvatarDialogImpl(this, myConfig);
	mySettingsDialog = new settingsDialogImpl(this, myConfig, mySelectAvatarDialog);
	myJoinNetworkGameDialog = new joinNetworkGameDialogImpl(this, myConfig);
	myConnectToServerDialog = new connectToServerDialogImpl(this);
	myStartNetworkGameDialog = new startNetworkGameDialogImpl(this, myConfig);
	myCreateNetworkGameDialog = new createNetworkGameDialogImpl(this, myConfig);
	myAboutPokerthDialog = new aboutPokerthImpl(this, myConfig);
	myGameLobbyDialog = new gameLobbyDialogImpl(this, myConfig);

	myStartNetworkGameDialog->setMyW(myGuiInterface->getMyW());
	myGameLobbyDialog->setMyW(myGuiInterface->getMyW());

	myTimeoutDialog = new timeoutMsgBoxImpl(this);
	myServerListDialog = new serverListDialogImpl(this, this, myConfig);
	myInternetGameLoginDialog = new internetGameLoginDialogImpl(this, myConfig);

	connect( actionStart_Local_Game, SIGNAL( triggered() ), this, SLOT( callNewGameDialog() ) );
	connect( pushButtonStart_Local_Game, SIGNAL( clicked() ), this, SLOT( callNewGameDialog() ) );
	connect( actionInternet_Game, SIGNAL( triggered() ), this, SLOT( joinGameLobby() ) );
	connect( pushButtonInternet_Game, SIGNAL( clicked() ), this, SLOT( joinGameLobby() ) );
	connect( actionCreate_Network_Game, SIGNAL( triggered() ), this, SLOT( callCreateNetworkGameDialog() ) );
	connect( pushButton_Create_Network_Game, SIGNAL( clicked() ), this, SLOT( callCreateNetworkGameDialog() ) );
	connect( actionJoin_Network_Game, SIGNAL( triggered() ), this, SLOT( callJoinNetworkGameDialog() ) );
	connect( pushButton_Join_Network_Game, SIGNAL( clicked() ), this, SLOT( callJoinNetworkGameDialog() ) );

	connect( actionAbout_PokerTH, SIGNAL( triggered() ), this, SLOT( callAboutPokerthDialog() ) );
	connect( actionConfigure_PokerTH, SIGNAL( triggered() ), this, SLOT( callSettingsDialog() ) );


	connect(this, SIGNAL(signalShowClientDialog()), this, SLOT(showClientDialog()));

	connect(this, SIGNAL(signalNetClientConnect(int)), myConnectToServerDialog, SLOT(refresh(int)));
	connect(this, SIGNAL(signalNetClientGameInfo(int)), myStartNetworkGameDialog, SLOT(refresh(int)));
	connect(this, SIGNAL(signalNetClientGameInfo(int)), myGameLobbyDialog, SLOT(refresh(int)));

	connect(this, SIGNAL(signalNetClientServerListShow()), myServerListDialog, SLOT(exec()));
	connect(this, SIGNAL(signalNetClientServerListClear()), myServerListDialog, SLOT(clearList()));
	connect(this, SIGNAL(signalNetClientServerListAdd(unsigned)), myServerListDialog, SLOT(addServerItem(unsigned)));

	connect(this, SIGNAL(signalNetClientLoginShow()), this, SLOT(callInternetGameLoginDialog()));

	connect(this, SIGNAL(signalNetClientSelfJoined(unsigned, QString, bool)), myStartNetworkGameDialog, SLOT(joinedNetworkGame(unsigned, QString, bool)));
	connect(this, SIGNAL(signalNetClientPlayerJoined(unsigned, QString, bool)), myStartNetworkGameDialog, SLOT(addConnectedPlayer(unsigned, QString, bool)));
	connect(this, SIGNAL(signalNetClientPlayerChanged(unsigned, QString)), myStartNetworkGameDialog, SLOT(updatePlayer(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientPlayerLeft(unsigned, QString)), myStartNetworkGameDialog, SLOT(removePlayer(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientNewGameAdmin(unsigned, QString)), myStartNetworkGameDialog, SLOT(newGameAdmin(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientGameListNew(unsigned)), myStartNetworkGameDialog, SLOT(gameCreated(unsigned)));

	connect(this, SIGNAL(signalNetClientSelfJoined(unsigned, QString, bool)), myGameLobbyDialog, SLOT(joinedNetworkGame(unsigned, QString, bool)));
	connect(this, SIGNAL(signalNetClientPlayerJoined(unsigned, QString, bool)), myGameLobbyDialog, SLOT(addConnectedPlayer(unsigned, QString, bool)));
	connect(this, SIGNAL(signalNetClientPlayerChanged(unsigned, QString)), myGameLobbyDialog, SLOT(updatePlayer(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientPlayerLeft(unsigned, QString)), myGameLobbyDialog, SLOT(removePlayer(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientNewGameAdmin(unsigned, QString)), myGameLobbyDialog, SLOT(newGameAdmin(unsigned, QString)));

	connect(this, SIGNAL(signalNetClientGameListNew(unsigned)), myGameLobbyDialog, SLOT(addGame(unsigned)));
	connect(this, SIGNAL(signalNetClientGameListRemove(unsigned)), myGameLobbyDialog, SLOT(removeGame(unsigned)));
	connect(this, SIGNAL(signalNetClientGameListUpdateMode(unsigned, int)), myGameLobbyDialog, SLOT(updateGameMode(unsigned, int)));
	connect(this, SIGNAL(signalNetClientGameListUpdateAdmin(unsigned, unsigned)), myGameLobbyDialog, SLOT(updateGameAdmin(unsigned, unsigned)));
	connect(this, SIGNAL(signalNetClientGameListPlayerJoined(unsigned, unsigned)), myGameLobbyDialog, SLOT(gameAddPlayer(unsigned, unsigned)));
	connect(this, SIGNAL(signalNetClientGameListPlayerLeft(unsigned, unsigned)), myGameLobbyDialog, SLOT(gameRemovePlayer(unsigned, unsigned)));
	connect(this, SIGNAL(signalNetClientRemovedFromGame(int)), myGameLobbyDialog, SLOT(removedFromGame(int)));
	connect(this, SIGNAL(signalNetClientStatsUpdate(ServerStats)), myGameLobbyDialog, SLOT(updateStats(ServerStats)));

	connect(this, SIGNAL(signalNetClientGameChatMsg(QString, QString)), myGuiInterface->getMyW()->getMyChat(), SLOT(receiveMessage(QString, QString)));
	connect(this, SIGNAL(signalNetClientLobbyChatMsg(QString, QString)), myStartNetworkGameDialog->getMyChat(), SLOT(receiveMessage(QString, QString)));
	connect(this, SIGNAL(signalNetClientLobbyChatMsg(QString, QString)), myGameLobbyDialog->getMyChat(), SLOT(receiveMessage(QString, QString)));
	connect(this, SIGNAL(signalNetClientMsgBox(QString)), this, SLOT(networkMessage(QString)));
	connect(this, SIGNAL(signalNetClientMsgBox(unsigned)), this, SLOT(networkMessage(unsigned)));
	connect(this, SIGNAL(signalNetClientShowTimeoutDialog(int, unsigned)), this, SLOT(showTimeoutDialog(int, unsigned)));

	connect(this, SIGNAL(signalLobbyPlayerJoined(unsigned, QString)), myGameLobbyDialog, SLOT(playerJoinedLobby(unsigned, QString)));
	connect(this, SIGNAL(signalLobbyPlayerLeft(unsigned)), myGameLobbyDialog, SLOT(playerLeftLobby(unsigned)));


	// Errors are handled globally, not within one dialog.
	connect(this, SIGNAL(signalNetClientError(int, int)), this, SLOT(networkError(int, int)));
	connect(this, SIGNAL(signalNetClientNotification(int)), this, SLOT(networkNotification(int)));
	connect(this, SIGNAL(signalNetServerError(int, int)), this, SLOT(networkError(int, int)));
	connect(this, SIGNAL(signalNetClientRemovedFromGame(int)), this, SLOT(networkNotification(int)));
	connect(this, SIGNAL(signalNetClientGameStart(boost::shared_ptr<Game>)), this, SLOT(networkStart(boost::shared_ptr<Game>)));

	connect(this, SIGNAL(signalSelfGameInvitation(unsigned, unsigned)), myGameLobbyDialog, SLOT(showInvitationDialog(unsigned, unsigned)));
	connect(this, SIGNAL(signalPlayerGameInvitation(unsigned, unsigned, unsigned)), myGameLobbyDialog, SLOT(chatInfoPlayerInvitation(unsigned, unsigned, unsigned)));
	connect(this, SIGNAL(signalRejectedGameInvitation(unsigned, unsigned, DenyGameInvitationReason)), myGameLobbyDialog, SLOT(chatInfoPlayerRejectedInvitation(unsigned, unsigned, DenyGameInvitationReason)));

	this->show();

	//update HACKS
	if(!checkForFirstStartAfterUpdated().isEmpty()) {
		qDebug() << checkForFirstStartAfterUpdated();
	}

}

startWindowImpl::~startWindowImpl()
{
}

void startWindowImpl::callNewGameDialog()
{

	//wenn Dialogfenster gezeigt werden soll
	if(myConfig->readConfigInt("ShowGameSettingsDialogOnNewGame")) {

		myNewGameDialog->exec();
		if (myNewGameDialog->result() == QDialog::Accepted ) {
			startNewLocalGame(myNewGameDialog);
		}
	}
	// sonst mit gespeicherten Werten starten
	else {
		startNewLocalGame();
	}
}

void startWindowImpl::startNewLocalGame(newGameDialogImpl *v)
{

	this->hide();
	myGuiInterface->getMyW()->show();

	// Start new local game - terminate existing network game.
	mySession->terminateNetworkClient();
	if (myServerGuiInterface.get())
		myServerGuiInterface->getSession()->terminateNetworkServer();

	//get values from local game dialog
	GameData gameData;
	if(v) {
		// Set Game Data
		gameData.maxNumberOfPlayers = v->spinBox_quantityPlayers->value();
		gameData.startMoney = v->spinBox_startCash->value();
		gameData.firstSmallBlind = v->getChangeCompleteBlindsDialog()->spinBox_firstSmallBlind->value();

		if(v->getChangeCompleteBlindsDialog()->radioButton_raiseBlindsAtHands->isChecked()) {
			gameData.raiseIntervalMode = RAISE_ON_HANDNUMBER;
			gameData.raiseSmallBlindEveryHandsValue = v->getChangeCompleteBlindsDialog()->spinBox_raiseSmallBlindEveryHands->value();
		} else {
			gameData.raiseIntervalMode = RAISE_ON_MINUTES;
			gameData.raiseSmallBlindEveryMinutesValue = v->getChangeCompleteBlindsDialog()->spinBox_raiseSmallBlindEveryMinutes->value();
		}

		if(v->getChangeCompleteBlindsDialog()->radioButton_alwaysDoubleBlinds->isChecked()) {
			gameData.raiseMode = DOUBLE_BLINDS;
		} else {
			gameData.raiseMode = MANUAL_BLINDS_ORDER;
			list<int> tempBlindList;
			int i;
			bool ok = TRUE;
			for(i=0; i<v->getChangeCompleteBlindsDialog()->listWidget_blinds->count(); i++) {
				tempBlindList.push_back(v->getChangeCompleteBlindsDialog()->listWidget_blinds->item(i)->text().toInt(&ok,10));
			}
			gameData.manualBlindsList = tempBlindList;

			if(v->getChangeCompleteBlindsDialog()->radioButton_afterThisAlwaysDoubleBlinds->isChecked()) {
				gameData.afterManualBlindsMode = AFTERMB_DOUBLE_BLINDS;
			} else {
				if(v->getChangeCompleteBlindsDialog()->radioButton_afterThisAlwaysRaiseAbout->isChecked()) {
					gameData.afterManualBlindsMode = AFTERMB_RAISE_ABOUT;
					gameData.afterMBAlwaysRaiseValue = v->getChangeCompleteBlindsDialog()->spinBox_afterThisAlwaysRaiseValue->value();
				} else {
					gameData.afterManualBlindsMode = AFTERMB_STAY_AT_LAST_BLIND;
				}
			}
		}

		//Speeds
		gameData.guiSpeed = v->spinBox_gameSpeed->value();
	}
	// start with default values
	else {
		// Set Game Data
		gameData.maxNumberOfPlayers = myConfig->readConfigInt("NumberOfPlayers");
		gameData.startMoney = myConfig->readConfigInt("StartCash");
		gameData.firstSmallBlind =  myConfig->readConfigInt("FirstSmallBlind");

		if(myConfig->readConfigInt("RaiseBlindsAtHands")) {
			gameData.raiseIntervalMode = RAISE_ON_HANDNUMBER;
			gameData.raiseSmallBlindEveryHandsValue = myConfig->readConfigInt("RaiseSmallBlindEveryHands");
		} else {
			gameData.raiseIntervalMode = RAISE_ON_MINUTES;
			gameData.raiseSmallBlindEveryMinutesValue = myConfig->readConfigInt("RaiseSmallBlindEveryMinutes");
		}

		if(myConfig->readConfigInt("AlwaysDoubleBlinds")) {
			gameData.raiseMode = DOUBLE_BLINDS;
		} else {
			gameData.raiseMode = MANUAL_BLINDS_ORDER;
			gameData.manualBlindsList = myConfig->readConfigIntList("ManualBlindsList");

			if(myConfig->readConfigInt("AfterMBAlwaysDoubleBlinds")) {
				gameData.afterManualBlindsMode = AFTERMB_DOUBLE_BLINDS;
			} else {
				if(myConfig->readConfigInt("AfterMBAlwaysRaiseAbout")) {
					gameData.afterManualBlindsMode = AFTERMB_RAISE_ABOUT;
					gameData.afterMBAlwaysRaiseValue = myConfig->readConfigInt("AfterMBAlwaysRaiseValue");
				} else {
					gameData.afterManualBlindsMode = AFTERMB_STAY_AT_LAST_BLIND;
				}
			}
		}
		//Speeds
		gameData.guiSpeed = myConfig->readConfigInt("GameSpeed");
	}
	// Set dealer pos.
	StartData startData;
	int tmpDealerPos = 0;
	startData.numberOfPlayers = gameData.maxNumberOfPlayers;
	Tools::getRandNumber(0, startData.numberOfPlayers-1, 1, &tmpDealerPos, 0);
	//if(DEBUG_MODE) {
	//    tmpDealerPos = 4;
	//}
	startData.startDealerPlayerId = static_cast<unsigned>(tmpDealerPos);

	//some gui modifications
	myGuiInterface->getMyW()->localGameModification();

	//Start Game!!!
	mySession->startLocalGame(gameData, startData);
}

void startWindowImpl::callGameLobbyDialog()
{

	//Avoid join Lobby with "Human Player" nick
	if(QString::fromUtf8(myConfig->readConfigString("MyName").c_str()) == QString("Human Player")) {
		changeContentDialogImpl dialog(this, myConfig, CHANGE_HUMAN_PLAYER_NAME);
		dialog.exec();
		if(dialog.result() == QDialog::Accepted) {
			joinGameLobby();
		}
	} else {
		joinGameLobby();
	}
}

void startWindowImpl::joinGameLobby()
{

	// Stop local game.
	myGuiInterface->getMyW()->stopTimer();

	mySession->terminateNetworkClient();
	if (myServerGuiInterface)
		myServerGuiInterface->getSession()->terminateNetworkServer();

	myGameLobbyDialog->setSession(getSession());
	myStartNetworkGameDialog->setSession(getSession());

	// Clear Lobby dialog.
	myGameLobbyDialog->clearDialog();

	//start internet client with config values for user and pw TODO
	mySession->startInternetClient();

	//Dialog mit Statusbalken
	myConnectToServerDialog->exec();
	if (myConnectToServerDialog->result() == QDialog::Accepted ) {
		showLobbyDialog();
	} else {
		mySession->terminateNetworkClient();
	}
}


void startWindowImpl::callInternetGameLoginDialog()
{

	//login

	//HACK if Outdated Version info is available show it here!
	if(msgBoxOutdatedVersionActive) {
		msgBoxOutdatedVersionActive = false;
		msgBoxOutdatedVersion.exec();
		msgBoxOutdatedVersion.raise();
		msgBoxOutdatedVersion.activateWindow();
	}

	myInternetGameLoginDialog->exec();

	if(myInternetGameLoginDialog->result() == QDialog::Accepted) {
		//send login infos
		mySession->setLogin(
			myConfig->readConfigString("MyName"),
			myInternetGameLoginDialog->lineEdit_password->text().toUtf8().constData(),
			myInternetGameLoginDialog->checkBox_guest->isChecked());
	} else {
		myConnectToServerDialog->reject();
		mySession->terminateNetworkClient();
	}
}

void startWindowImpl::callCreateNetworkGameDialog()
{

	myCreateNetworkGameDialog->exec();
	//
	if (myCreateNetworkGameDialog->result() == QDialog::Accepted ) {

		// Stop local game.
		myGuiInterface->getMyW()->stopTimer();

		if (!myServerGuiInterface) {
			// Create pseudo Gui Wrapper for the server.
			myServerGuiInterface.reset(new ServerGuiWrapper(myConfig, mySession->getGui(), mySession->getGui(), mySession->getGui()));
			{
				boost::shared_ptr<Session> session(new Session(myServerGuiInterface.get(), myConfig));
				session->init(mySession->getAvatarManager());
				myServerGuiInterface->setSession(session);
			}
		}

		// Terminate existing network games.
		mySession->terminateNetworkClient();
		myServerGuiInterface->getSession()->terminateNetworkServer();

		GameData gameData;
		gameData.maxNumberOfPlayers = myCreateNetworkGameDialog->spinBox_quantityPlayers->value();
		gameData.startMoney = myCreateNetworkGameDialog->spinBox_startCash->value();
		gameData.firstSmallBlind = myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->spinBox_firstSmallBlind->value();

		if(myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->radioButton_raiseBlindsAtHands->isChecked()) {
			gameData.raiseIntervalMode = RAISE_ON_HANDNUMBER;
			gameData.raiseSmallBlindEveryHandsValue = myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->spinBox_raiseSmallBlindEveryHands->value();
		} else {
			gameData.raiseIntervalMode = RAISE_ON_MINUTES;
			gameData.raiseSmallBlindEveryMinutesValue = myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->spinBox_raiseSmallBlindEveryMinutes->value();
		}

		if(myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->radioButton_alwaysDoubleBlinds->isChecked()) {
			gameData.raiseMode = DOUBLE_BLINDS;
		} else {
			gameData.raiseMode = MANUAL_BLINDS_ORDER;
			std::list<int> tempBlindList;
			int i;
			bool ok = TRUE;
			for(i=0; i<myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->listWidget_blinds->count(); i++) {
				tempBlindList.push_back(myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->listWidget_blinds->item(i)->text().toInt(&ok,10));
			}
			gameData.manualBlindsList = tempBlindList;

			if(myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->radioButton_afterThisAlwaysDoubleBlinds->isChecked()) {
				gameData.afterManualBlindsMode = AFTERMB_DOUBLE_BLINDS;
			} else {
				if(myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->radioButton_afterThisAlwaysRaiseAbout->isChecked()) {
					gameData.afterManualBlindsMode = AFTERMB_RAISE_ABOUT;
					gameData.afterMBAlwaysRaiseValue = myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->spinBox_afterThisAlwaysRaiseValue->value();
				} else {
					gameData.afterManualBlindsMode = AFTERMB_STAY_AT_LAST_BLIND;
				}
			}
		}

		gameData.guiSpeed = myConfig->readConfigInt("GameSpeed");
		gameData.delayBetweenHandsSec = myCreateNetworkGameDialog->spinBox_netDelayBetweenHands->value();
		gameData.playerActionTimeoutSec = myCreateNetworkGameDialog->spinBox_netTimeOutPlayerAction->value();

		myGameLobbyDialog->setSession(getSession());
		myStartNetworkGameDialog->setSession(getSession());

		// Clear network game dialog.
		myStartNetworkGameDialog->clearDialog();

		myServerGuiInterface->getSession()->startNetworkServer(false);
		mySession->startNetworkClientForLocalServer(gameData);

		myStartNetworkGameDialog->setMaxPlayerNumber(gameData.maxNumberOfPlayers);

		myStartNetworkGameDialog->setWindowTitle("Start Network Game");

		showNetworkStartDialog();
	}

}

void startWindowImpl::callJoinNetworkGameDialog()
{

	myJoinNetworkGameDialog->exec();

	if (myJoinNetworkGameDialog->result() == QDialog::Accepted ) {

		// Stop local game.
		myGuiInterface->getMyW()->stopTimer();

		mySession->terminateNetworkClient();
		if (myServerGuiInterface)
			myServerGuiInterface->getSession()->terminateNetworkServer();

		myGameLobbyDialog->setSession(getSession());
		myStartNetworkGameDialog->setSession(getSession());
		// Clear network game dialog
		myStartNetworkGameDialog->clearDialog();
		// Maybe use QUrl::toPunycode.
		mySession->startNetworkClient(
			myJoinNetworkGameDialog->lineEdit_ipAddress->text().toUtf8().constData(),
			myJoinNetworkGameDialog->spinBox_port->value(),
			myJoinNetworkGameDialog->checkBox_ipv6->isChecked(),
			myJoinNetworkGameDialog->checkBox_sctp->isChecked());

		//Dialog mit Statusbalken
		myConnectToServerDialog->exec();

		if (myConnectToServerDialog->result() == QDialog::Rejected ) {
			mySession->terminateNetworkClient();
			actionJoin_Network_Game->trigger(); // re-trigger
		} else {
			//needed for join and ready sounds - TODO
			//myStartNetworkGameDialog->setMaxPlayerNumber(gameData.maxNumberOfPlayers);
			myStartNetworkGameDialog->setWindowTitle("Start Network Game");

			showNetworkStartDialog();
		}
	}
}


void startWindowImpl::showClientDialog()
{
	if (mySession->getGameType() == Session::GAME_TYPE_NETWORK) {
		if (myGuiInterface->getMyW()->isVisible())
			myGuiInterface->getMyW()->hide();
		if (!this->isVisible())
			this->show();
		if (!myStartNetworkGameDialog->isVisible())
			showNetworkStartDialog();
	} else if (mySession->getGameType() == Session::GAME_TYPE_INTERNET) {
		if (myGuiInterface->getMyW()->isVisible())
			myGuiInterface->getMyW()->closeMessageBoxes();
		myGuiInterface->getMyW()->hide();
		if (!this->isVisible())
			this->show();
		if (!myGameLobbyDialog->isVisible())
			showLobbyDialog();
	}
}

void startWindowImpl::showLobbyDialog()
{
	myGameLobbyDialog->exec();

	if (myGameLobbyDialog->result() == QDialog::Accepted ) {
		this->hide();
		//some gui modifications
		myGuiInterface->getMyW()->networkGameModification();
	} else {
		myGameLobbyDialog->clearDialog();
		mySession->terminateNetworkClient();
	}
}

void startWindowImpl::showNetworkStartDialog()
{
	myStartNetworkGameDialog->exec();

	if (myStartNetworkGameDialog->result() == QDialog::Accepted ) {

		this->hide();
		//some gui modifications
		myGuiInterface->getMyW()->networkGameModification();
	} else {
		mySession->terminateNetworkClient();
		if (myServerGuiInterface)
			myServerGuiInterface->getSession()->terminateNetworkServer();
	}
}

void startWindowImpl::callAboutPokerthDialog()
{
	myAboutPokerthDialog->exec();
}

void startWindowImpl::callSettingsDialog()
{

	mySettingsDialog->exec();

	if (mySettingsDialog->result() == QDialog::Accepted && mySettingsDialog->getSettingsCorrect()) {
		myGuiInterface->getMyW()->applySettings(mySettingsDialog);
	}
}

void startWindowImpl::showTimeoutDialog(int msgID, unsigned duration)
{

	if(myTimeoutDialog->isHidden()) {
		myTimeoutDialog->setMySession(mySession);
		myTimeoutDialog->setMsgID((NetTimeoutReason)msgID);
		myTimeoutDialog->setTimeoutDuration(duration);
		myTimeoutDialog->show();
		myTimeoutDialog->raise();
		myTimeoutDialog->activateWindow();
		myTimeoutDialog->startTimeout();
	}
}

void startWindowImpl::hideTimeoutDialog()
{
	myTimeoutDialog->hide();
}

void startWindowImpl::networkError(int errorID, int /*osErrorID*/)
{

	hideTimeoutDialog();
	switch (errorID) {
	case ERR_SOCK_SERVERADDR_NOT_SET: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Server address was not set."),
							 QMessageBox::Close);
	}
	break;
	case ERR_SOCK_INVALID_PORT: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("An invalid port was set (ports 0-1023 are not allowed)."),
							 QMessageBox::Close);
	}
	break;
	case ERR_SOCK_CREATION_FAILED: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Could not create a socket for TCP communication."),
							 QMessageBox::Close);
	}
	break;
	case ERR_SOCK_SET_ADDR_FAILED: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Could not set the IP address."),
							 QMessageBox::Close);
	}
	break;
	case ERR_SOCK_SET_PORT_FAILED: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Could not set the port for this type of address."),
							 QMessageBox::Close);
	}
	break;
	case ERR_SOCK_RESOLVE_FAILED: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("The server name could not be resolved."),
							 QMessageBox::Close);
	}
	break;
	case ERR_SOCK_BIND_FAILED: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Bind failed - please choose a different port."),
							 QMessageBox::Close);
	}
	break;
	case ERR_SOCK_LISTEN_FAILED: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Internal network error: \"listen\" failed."),
							 QMessageBox::Close);
	}
	break;
	case ERR_SOCK_ACCEPT_FAILED: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Server execution was terminated."),
							 QMessageBox::Close);
	}
	break;
	case ERR_SOCK_CONNECT_FAILED: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Could not connect to the server."),
							 QMessageBox::Close);
	}
	break;
	case ERR_SOCK_CONNECT_TIMEOUT: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Connection timed out.\nPlease check the server address.\n\nIf the server is behind a NAT-Router, make sure port forwarding has been set up on server side."),
							 QMessageBox::Close);
	}
	break;
	case ERR_SOCK_SELECT_FAILED: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Internal network error: \"select\" failed."),
							 QMessageBox::Close);
	}
	break;
	case ERR_SOCK_SEND_FAILED: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Internal network error: \"send\" failed."),
							 QMessageBox::Close);
	}
	break;
	case ERR_SOCK_RECV_FAILED: // Sometimes windows reports recv failed on close.
	case ERR_SOCK_CONN_RESET: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Connection was closed by the server."),
							 QMessageBox::Close);
	}
	break;
	case ERR_SOCK_CONN_EXISTS: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Internal network error: Duplicate TCP connection."),
							 QMessageBox::Close);
	}
	break;
	case ERR_SOCK_INVALID_PACKET: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("An invalid network packet was received.\nPlease make sure that all players use the same version of PokerTH."),
							 QMessageBox::Close);
	}
	break;
	case ERR_SOCK_INVALID_STATE: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Internal state error.\nPlease make sure that all players use the same version of PokerTH."),
							 QMessageBox::Close);
	}
	break;
	case ERR_SOCK_INVALID_SERVERLIST_URL:
	case ERR_SOCK_TRANSFER_INVALID_URL: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Invalid server list URL.\nPlease correct the address in the settings."),
							 QMessageBox::Close);
	}
	break;
	case ERR_SOCK_OPEN_MD5_FAILED: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Could not open the server list MD5 file.\nPlease make sure that the server list URL is correct."),
							 QMessageBox::Close);
	}
	break;
	case ERR_SOCK_INVALID_SERVERLIST_MD5: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Synchronization of the PokerTH internet server list has failed.\nPlease make sure that the server list URL is correct."),
							 QMessageBox::Close);
	}
	break;
	case ERR_SOCK_INVALID_SERVERLIST_XML: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("The PokerTH internet server list contains invalid data.\nIf you use a custom server list, please make sure its format is correct."),
							 QMessageBox::Close);
	}
	break;
	case ERR_SOCK_UNZIP_FAILED: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Could not unzip the PokerTH internet server list."),
							 QMessageBox::Close);
	}
	break;
	case ERR_SOCK_TRANSFER_INIT_FAILED:
	case ERR_SOCK_TRANSFER_SELECT_FAILED:
	case ERR_SOCK_TRANSFER_FAILED: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Could not download the PokerTH internet server list.\nPlease make sure you are directly connected to the internet."),
							 QMessageBox::Close);
	}
	break;
	case ERR_SOCK_TRANSFER_OPEN_FAILED: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Could not open the target file when downloading the server list."),
							 QMessageBox::Close);
	}
	break;
	case ERR_NET_VERSION_NOT_SUPPORTED: {
		QMessageBox msgBox(QMessageBox::Warning, tr("Network Error"),
						   tr("The PokerTH server does not support this version of the game.<br>Please go to <a href=\"http://www.pokerth.net/\" target=\"_blank\">http://www.pokerth.net</a> and download the latest version."),
						   QMessageBox::Close, this);
		msgBox.setTextFormat(Qt::RichText);
		msgBox.exec();
	}
	break;
	case ERR_NET_SERVER_FULL: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Sorry, this server is already full."),
							 QMessageBox::Close);
	}
	break;
	case ERR_NET_INVALID_PASSWORD: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Invalid login.\nPlease check your username and password."),
							 QMessageBox::Close);
	}
	break;
	case ERR_NET_INVALID_PASSWORD_STR: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("The password is too long. Please choose another one."),
							 QMessageBox::Close);
	}
	break;
	case ERR_NET_PLAYER_NAME_IN_USE: {
		changeContentDialogImpl dialog(this, myConfig, CHANGE_NICK_ALREADY_IN_USE);
		dialog.exec();
	}
	break;
	case ERR_NET_INVALID_PLAYER_NAME: {
		changeContentDialogImpl dialog(this, myConfig, CHANGE_NICK_INVALID);
		dialog.exec();
	}
	break;
	case ERR_NET_INVALID_GAME_NAME: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("The game name is either too short or too long. Please choose another one."),
							 QMessageBox::Close);
	}
	break;
	case ERR_NET_UNKNOWN_GAME: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("The game could not be found."),
							 QMessageBox::Close);
	}
	break;
	case ERR_NET_INVALID_CHAT_TEXT: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("The chat text is invalid."),
							 QMessageBox::Close);
	}
	break;
	case ERR_NET_UNKNOWN_PLAYER_ID: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("The server referred to an unknown player. Aborting."),
							 QMessageBox::Close);
	}
	break;
	case ERR_NET_NO_CURRENT_PLAYER: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Internal error: The current player could not be found."),
							 QMessageBox::Close);
	}
	break;
	case ERR_NET_PLAYER_NOT_ACTIVE: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Internal error: The current player is not active."),
							 QMessageBox::Close);
	}
	break;
	case ERR_NET_PLAYER_KICKED: {
		mySession->terminateNetworkClient();
		QMessageBox::warning(this, tr("Network Error"),
							 tr("You were kicked from the server."),
							 QMessageBox::Close);
	}
	break;
	case ERR_NET_PLAYER_BANNED: {
		mySession->terminateNetworkClient();
		QMessageBox::warning(this, tr("Network Error"),
							 tr("You were temporarily banned from the server."),
							 QMessageBox::Close);
	}
	break;
	case ERR_NET_PLAYER_BLOCKED: {
		mySession->terminateNetworkClient();
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Your account is blocked indefinitely."),
							 QMessageBox::Close);
	}
	break;
	case ERR_NET_SESSION_TIMED_OUT: {
		mySession->terminateNetworkClient();
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Your server connection timed out due to inactivity. You are very welcome to reconnect!"),
							 QMessageBox::Close);
	}
	break;
	case ERR_NET_INVALID_PLAYER_COUNT: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("The client player count is invalid."),
							 QMessageBox::Close);
	}
	break;
	case ERR_NET_TOO_MANY_MANUAL_BLINDS: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Too many manual blinds were set. Please reconfigure the manual blinds."),
							 QMessageBox::Close);
	}
	break;
	case ERR_NET_INVALID_AVATAR_FILE:
	case ERR_NET_WRONG_AVATAR_SIZE: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("An invalid avatar file was configured. Please choose a different avatar."),
							 QMessageBox::Close);
	}
	break;
	case ERR_NET_AVATAR_TOO_LARGE: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("The selected avatar file is too large. Please choose a different avatar."),
							 QMessageBox::Close);
	}
	break;
	case ERR_NET_INIT_BLOCKED: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("You cannot login at this time. Please try again in a few seconds."),
							 QMessageBox::Close);
	}
	break;
	case ERR_NET_INVALID_REQUEST_ID: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("An internal avatar error occured. Please report this to an admin in the lobby chat."),
							 QMessageBox::Close);
	}
	break;
	case ERR_NET_START_TIMEOUT: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Could not start game: Synchronization failed."),
							 QMessageBox::Close);
	}
	break;
	case ERR_NET_SERVER_MAINTENANCE: {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("The server is down for maintenance. Please try again later."),
							 QMessageBox::Close);
	}
	break;
	default:  {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("An internal error occured."),
							 QMessageBox::Close);
	}
	}
	// close dialogs
	myGameLobbyDialog->reject();
	myConnectToServerDialog->reject();
	myStartNetworkGameDialog->reject();
	myGuiInterface->getMyW()->close();
}

void startWindowImpl::networkNotification(int notificationId)
{
	hideTimeoutDialog();
	switch (notificationId) {
	case NTF_NET_JOIN_IP_BLOCKED: {
		QMessageBox::warning(this, tr("Network Notification"),
							 tr("You cannot join this game, because another player in that game has your network address."),
							 QMessageBox::Close);
	}
	break;
	case NTF_NET_REMOVED_START_FAILED: {
		QMessageBox::warning(this, tr("Network Notification"),
							 tr("Your connection to the server is very slow, the game had to start without you."),
							 QMessageBox::Close);
	}
	break;
	case NTF_NET_REMOVED_KICKED: {
		QMessageBox::warning(this, tr("Network Notification"),
							 tr("You were kicked from the game."),
							 QMessageBox::Close);
	}
	break;
	case NTF_NET_REMOVED_GAME_FULL:
	case NTF_NET_JOIN_GAME_FULL: {
		QMessageBox::warning(this, tr("Network Notification"),
							 tr("Sorry, this game is already full."),
							 QMessageBox::Close);
	}
	break;
	case NTF_NET_REMOVED_ALREADY_RUNNING:
	case NTF_NET_JOIN_ALREADY_RUNNING: {
		QMessageBox::warning(this, tr("Network Notification"),
							 tr("Unable to join - the server has already started the game."),
							 QMessageBox::Close);
	}
	break;
	case NTF_NET_JOIN_NOT_INVITED: {
		QMessageBox::warning(this, tr("Network Notification"),
							 tr("This game is of type invite-only. You cannot join this game without being invited."),
							 QMessageBox::Close);
	}
	break;
	case NTF_NET_JOIN_GAME_NAME_IN_USE: {
		changeContentDialogImpl dialog(this, myConfig, CHANGE_INET_GAME_NAME_IN_USE);
		dialog.exec();
		if(dialog.result() == QDialog::Accepted) {
			myGameLobbyDialog->pushButton_CreateGame->click();
		}
	}
	break;
	case NTF_NET_JOIN_GAME_BAD_NAME: {
		changeContentDialogImpl dialog(this, myConfig, CHANGE_INET_BAD_GAME_NAME);
		dialog.exec();
		if(dialog.result() == QDialog::Accepted) {
			myGameLobbyDialog->pushButton_CreateGame->click();
		}
	}
	break;
	case NTF_NET_REMOVED_TIMEOUT: {
		QMessageBox::warning(this, tr("Network Notification"),
							 tr("Your admin state timed out due to inactivity. Feel free to create a new game!"),
							 QMessageBox::Close);
	}
	break;
	case NTF_NET_JOIN_INVALID_PASSWORD: {
		QMessageBox::warning(this, tr("Network Notification"),
							 tr("Invalid password when joining the game.\nPlease reenter the password and try again."),
							 QMessageBox::Close);
	}
	break;
	case NTF_NET_JOIN_GUEST_FORBIDDEN: {
		QMessageBox::warning(this, tr("Network Notification"),
							 tr("You cannot join this type of game as guest."),
							 QMessageBox::Close);
	}
	break;
	case NTF_NET_JOIN_INVALID_SETTINGS: {
		QMessageBox::warning(this, tr("Network Notification"),
							 tr("The settings are invalid for this type of game."),
							 QMessageBox::Close);
	}
	break;
	case NTF_NET_NEW_RELEASE_AVAILABLE: {
		msgBoxOutdatedVersion.setIcon(QMessageBox::Information);
		msgBoxOutdatedVersion.setWindowTitle(tr("Network Notification"));
		msgBoxOutdatedVersion.setText(tr("A new release of PokerTH is available.<br>Please go to <a href=\"http://www.pokerth.net/\" target=\"_blank\">http://www.pokerth.net</a> and download the latest version."));
		msgBoxOutdatedVersion.setTextFormat(Qt::RichText);
		msgBoxOutdatedVersion.setStandardButtons(QMessageBox::Ok);
		msgBoxOutdatedVersion.setDefaultButton(QMessageBox::Ok);

		msgBoxOutdatedVersionActive = true;
	}
	break;
	case NTF_NET_OUTDATED_BETA: {
		msgBoxOutdatedVersion.setIcon(QMessageBox::Information);
		msgBoxOutdatedVersion.setWindowTitle(tr("Network Notification"));
		msgBoxOutdatedVersion.setText(tr("This beta release of PokerTH is outdated.<br>Please go to <a href=\"http://www.pokerth.net/\" target=\"_blank\">http://www.pokerth.net</a> and download the latest version."));
		msgBoxOutdatedVersion.setTextFormat(Qt::RichText);
		msgBoxOutdatedVersion.setStandardButtons(QMessageBox::Ok);
		msgBoxOutdatedVersion.setDefaultButton(QMessageBox::Ok);

		msgBoxOutdatedVersionActive = true;
	}
	break;
	}
}

void startWindowImpl::networkMessage(QString msg)
{
	QMessageBox msgBox(QMessageBox::Information, tr("Server Message"),
					   msg, QMessageBox::Close, this);
	msgBox.setTextFormat(Qt::RichText);
	msgBox.exec();
}

void startWindowImpl::networkMessage(unsigned msgId)
{
	QString msgText;

	switch(msgId) {

	case MSG_NET_AVATAR_REPORT_ACCEPTED: {
		msgText = tr("The avatar report was accepted by the server. Thank you.");
	}
	break;
	case MSG_NET_AVATAR_REPORT_DUP: {
		msgText = tr("This avatar was already reported by another player.");
	}
	break;
	case MSG_NET_AVATAR_REPORT_REJECTED: {
		msgText = tr("An error occurred while reporting the avatar.");
	}
	break;
	default:
		;
		break;
	}

	QMessageBox msgBox(QMessageBox::Information, tr("Server Message"),
					   msgText, QMessageBox::Close, this);
//    msgBox.setTextFormat(Qt::RichText);
	msgBox.exec();
}


void startWindowImpl::networkStart(boost::shared_ptr<Game> game)
{
	mySession->startClientGame(game);

	//send playerNicksList to chat for nick-autocompletition
	myGuiInterface->getMyW()->getMyChat()->setPlayerNicksList(getPlayerNicksList());
}


QStringList startWindowImpl::getPlayerNicksList()
{

	QStringList list;
	PlayerListConstIterator it_c;
	PlayerList seatList = mySession->getCurrentGame()->getSeatsList();
	for (it_c=seatList->begin(); it_c!=seatList->end(); ++it_c) {

		list << QString::fromUtf8((*it_c)->getMyName().c_str());
	}

	return list;
}

QString startWindowImpl::checkForFirstStartAfterUpdated()
{
	if(myConfig->getConfigState() == OLD) {

		if(POKERTH_VERSION_MAJOR == 0 && POKERTH_VERSION_MINOR == 80) {
			//version 0.8 HACK
			//to avoid old PokerTH distributed styles pathes in settings which leads to error message like "outdated" prepare settings dialog (fallback will correct this issue) and save settings
			mySettingsDialog->prepareDialog();
			mySettingsDialog->isAccepted();
			myGuiInterface->getMyW()->applySettings(mySettingsDialog);
		}
		return QString("Update old config to version %1").arg(QString::number(POKERTH_VERSION_MAJOR)+"."+QString::number(POKERTH_VERSION_MINOR));
	}

	return QString();

}

bool startWindowImpl::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::Close) {
		event->ignore();
//        mySession->getMyLog()->closeLogDbAtExit();
		return QMainWindow::eventFilter(obj, event);
	} else {
		// pass the event on to the parent class
		return QMainWindow::eventFilter(obj, event);
	}
}
