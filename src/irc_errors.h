// This code is licensed under MIT license (see LICENSE.txt for details)
#pragma once

namespace irclib {

constexpr int ERR_UNKNOWNERROR           = 400;
constexpr int ERR_NOSUCHNICK             = 401;
constexpr int ERR_NOSUCHSERVER           = 402;
constexpr int ERR_NOSUCHCHANNEL          = 403;
constexpr int ERR_CANNOTSENDTOCHAN       = 404;
constexpr int ERR_TOOMANYCHANNELS        = 405;
constexpr int ERR_WASNOSUCHNICK          = 406;
constexpr int ERR_TOOMANYTARGETS         = 407;
constexpr int ERR_NOSUCHSERVICE          = 408;
constexpr int ERR_NOORIGIN               = 409;
constexpr int ERR_NORECIPIENT            = 411;
constexpr int ERR_NOTEXTTOSEND           = 412;
constexpr int ERR_NOTOPLEVEL             = 413;
constexpr int ERR_WILDTOPLEVEL           = 414;
constexpr int ERR_BADMASK                = 415;
constexpr int ERR_TOOMANYMATCHES         = 416;
constexpr int ERR_LENGTHTRUNCATED        = 419;
constexpr int ERR_UNKNOWNCOMMAND         = 421;
constexpr int ERR_NOMOTD                 = 422;
constexpr int ERR_NOADMININFO            = 423;
constexpr int ERR_FILEERROR              = 424;
constexpr int ERR_NOOPERMOTD             = 425;
constexpr int ERR_TOOMANYAWAY            = 429;
constexpr int ERR_EVENTNICKCHANGE        = 430;
constexpr int ERR_NONICKNAMEGIVEN        = 431;
constexpr int ERR_ERRONEUSNICKNAME       = 432;
constexpr int ERR_NICKNAMEINUSE          = 433;
constexpr int ERR_SERVICENAMEINUSE       = 434;
constexpr int ERR_BANONCHAN              = 435;
constexpr int ERR_NICKCOLLISION          = 436;
constexpr int ERR_BANNICKCHANGE          = 437;
constexpr int ERR_NICKTOOFAST            = 438;
constexpr int ERR_TARGETTOOFAST          = 439;
constexpr int ERR_SERVICESDOWN           = 440;
constexpr int ERR_USERNOTINCHANNEL       = 441;
constexpr int ERR_NOTONCHANNEL           = 442;
constexpr int ERR_USERONCHANNEL          = 443;
constexpr int ERR_NOLOGIN                = 444;
constexpr int ERR_SUMMONDISABLED         = 445;
constexpr int ERR_USERSDISABLED          = 446;
constexpr int ERR_NONICKCHANGE           = 447;
constexpr int ERR_NOTIMPLEMENTED         = 449;
constexpr int ERR_NOTREGISTERED          = 451;
constexpr int ERR_IDCOLLISION            = 452;
constexpr int ERR_NICKLOST               = 453;
constexpr int ERR_HOSTILENAME            = 455;
constexpr int ERR_ACCEPTFULL             = 456;
constexpr int ERR_ACCEPTEXIST            = 457;
constexpr int ERR_ACCEPTNOT              = 458;
constexpr int ERR_NOHIDING               = 459;
constexpr int ERR_NOTFORHALFOPS          = 460;
constexpr int ERR_NEEDMOREPARAMS         = 461;
constexpr int ERR_ALREADYREGISTERED      = 462;
constexpr int ERR_NOPERMFORHOST          = 463;
constexpr int ERR_PASSWDMISMATCH         = 464;
constexpr int ERR_YOUREBANNEDCREEP       = 465;
constexpr int ERR_YOUWILLBEBANNED        = 466;
constexpr int ERR_KEYSET                 = 467;
constexpr int ERR_INVALIDUSERNAME        = 468;
constexpr int ERR_LINKSET                = 469;
constexpr int ERR_KICKEDFROMCHAN         = 470;
constexpr int ERR_CHANNELISFULL          = 471;
constexpr int ERR_UNKNOWNMODE            = 472;
constexpr int ERR_INVITEONLYCHAN         = 473;
constexpr int ERR_BANNEDFROMCHAN         = 474;
constexpr int ERR_BADCHANNELKEY          = 475;
constexpr int ERR_BADCHANMASK            = 476;
constexpr int ERR_NOCHANMODES            = 477;
constexpr int ERR_BANLISTFULL            = 478;
constexpr int ERR_BADCHANNAME            = 479;
constexpr int ERR_NOULINE                = 480;
constexpr int ERR_NOPRIVILEGES           = 481;
constexpr int ERR_CHANOPRIVSNEEDED       = 482;
constexpr int ERR_CANTKILLSERVER         = 483;
constexpr int ERR_RESTRICTED             = 484;
constexpr int ERR_CANTKICKADMIN          = 485;
constexpr int ERR_NONONREG               = 486;
constexpr int ERR_CHANTOORECENT          = 487;
constexpr int ERR_TSLESSCHAN             = 488;
constexpr int ERR_VOICENEEDED            = 489;
constexpr int ERR_NOOPERHOST             = 491;
constexpr int ERR_NOSERVICEHOST          = 492;
constexpr int ERR_NOFEATURE              = 493;
constexpr int ERR_BADFEATURE             = 494;
constexpr int ERR_BADLOGTYPE             = 495;
constexpr int ERR_BADLOGSYS              = 496;
constexpr int ERR_BADLOGVALUE            = 497;
constexpr int ERR_ISOPERLCHAN            = 498;
constexpr int ERR_CHANOWNPRIVNEEDED      = 499;
constexpr int ERR_UMODEUNKNOWNFLAG       = 501;
constexpr int ERR_USERSDONTMATCH         = 502;
constexpr int ERR_GHOSTEDCLIENT          = 503;
constexpr int ERR_USERNOTONSERV          = 504;
constexpr int ERR_SILELISTFULL           = 511;
constexpr int ERR_TOOMANYWATCH           = 512;
constexpr int ERR_BADPING                = 513;
constexpr int ERR_INVALID_ERROR          = 514;
constexpr int ERR_BADEXPIRE              = 515;
constexpr int ERR_DONTCHEAT              = 516;
constexpr int ERR_DISABLED               = 517;
constexpr int ERR_NOINVITE               = 518;
constexpr int ERR_ADMONLY                = 519;
constexpr int ERR_OPERONLY               = 520;
constexpr int ERR_LISTSYNTAX             = 521;
constexpr int ERR_WHOSYNTAX              = 522;
constexpr int ERR_WHOLIMEXCEED           = 523;
constexpr int ERR_QUARANTINED            = 524;
constexpr int ERR_REMOTEPFX              = 525;
constexpr int ERR_PFXUNROUTABLE          = 526;
constexpr int ERR_BADHOSTMASK            = 550;
constexpr int ERR_HOSTUNAVAIL            = 551;
constexpr int ERR_USINGSLINE             = 552;
constexpr int ERR_STATSSLINE             = 553;
constexpr int ERR_TOOMANYKNOCK           = 712;
constexpr int ERR_CHANOPEN               = 713;
constexpr int ERR_KNOCKONCHAN            = 714;
constexpr int ERR_KNOCKDISABLED          = 715;
constexpr int ERR_NOPRIVS                = 723;
constexpr int ERR_CANNOTDOCOMMAND        = 972;
constexpr int ERR_CANNOTCHANGEUMODE      = 973;
constexpr int ERR_CANNOTCHANGECHANMODE   = 974;
constexpr int ERR_CANNOTCHANGESERVERMODE = 975;
constexpr int ERR_CANNOTSENDTONICK       = 976;
constexpr int ERR_UNKNOWNSERVERMODE      = 977;
constexpr int ERR_SERVERMODELOCK         = 979;
constexpr int ERR_BADCHARENCODING        = 980;
constexpr int ERR_TOOMANYLANGUAGES       = 981;
constexpr int ERR_NOLANGUAGE             = 982;
constexpr int ERR_TEXTTOOSHORT           = 983;
constexpr int ERR_NUMERIC_ERR            = 999;

} // namespace irclib
