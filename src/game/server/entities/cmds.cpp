#include <stdio.h>
#include <string.h>

#include <engine/shared/config.h>
#include <engine/server.h>
#include <game/version.h>
#include "cmds.h"
#include "account.h"
#include "hearth.h"

CCmd::CCmd(CPlayer *pPlayer, CGameContext *pGameServer)
{
	m_pPlayer = pPlayer;
	m_pGameServer = pGameServer;
}

void CCmd::ChatCmd(CNetMsg_Cl_Say *Msg)
{
	if(!strncmp(Msg->m_pMessage, "/login", 6))
	{
		LastChat();		
		if(GameServer()->m_World.m_Paused) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "请等待此回合结束！");
		
		char Username[256], Password[256];
		if(sscanf(Msg->m_pMessage, "/login %s %s", Username, Password) != 2) 
			return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "格式错误: /login <用户名> <密码>");
		
		if(str_length(Username) > 15 || str_length(Username) < 4 || str_length(Password) > 15 || str_length(Password) < 4) 
			return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "提示：用户名 / 密码必须在4-15个字母内");
		
		m_pPlayer->m_pAccount->Login(Username, Password);
		return;
	}
	
	else if(!strncmp(Msg->m_pMessage, "/register", 9))
	{
		LastChat();
		if(GameServer()->m_World.m_Paused)  return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "请等待此回合结束！");
		
		char Username[256], Password[256];
		if(sscanf(Msg->m_pMessage, "/register %s %s", Username, Password) != 2) 
			return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "格式错误：/register <用户名> <密码>");
		
		if(str_length(Username) > 15 || str_length(Username) < 4 || str_length(Password) > 15 || str_length(Password) < 4)
			return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "用户名和密码必须要有4-15个字母");
		else if (!strcmp(Username, Password))
			return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "用户名和密码不能相同");

		m_pPlayer->m_pAccount->Register(Username, Password);
		return;
	}
	
	else if (!strncmp(Msg->m_pMessage, "/sd", 3) && GameServer()->Server()->IsAuthed(m_pPlayer->GetCID()))
	{
		LastChat();
		int size = 0;
		if ((sscanf(Msg->m_pMessage, "/sd %d", &size)) != 1)
			return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "提示：/sd <声音ID>");

		if (m_pPlayer->GetTeam() == TEAM_SPECTATORS || !GameServer()->GetPlayerChar(m_pPlayer->GetCID()))
			return;

		int soundid = clamp(size, 0, 40);
		GameServer()->CreateSound(GameServer()->GetPlayerChar(m_pPlayer->GetCID())->m_Pos, soundid);
		return;
	}

	else if(!strcmp(Msg->m_pMessage, "/logout"))
	{
		LastChat();
		if(!m_pPlayer->m_AccData.m_UserID) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "你还没有登录！");
		if(GameServer()->m_World.m_Paused) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "请等待此回合结束");
		if (GameServer()->m_pController->NumZombs() == 1 && m_pPlayer->GetTeam() == TEAM_RED) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "作为第一只僵尸你不能退出登录");
		if (GameServer()->m_pController->NumPlayers() < 3 && GameServer()->m_pController->m_Warmup)	return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "请等待此回合开始！");

		m_pPlayer->m_pAccount->Apply(), m_pPlayer->m_pAccount->Reset();

		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "退出登录！");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "使用/login <用户名> <密码> 重新登录");

		if(GameServer()->GetPlayerChar(m_pPlayer->GetCID()) && GameServer()->GetPlayerChar(m_pPlayer->GetCID())->IsAlive())
			GameServer()->GetPlayerChar(m_pPlayer->GetCID())->Die(m_pPlayer->GetCID(), WEAPON_GAME);

		return;
	}	
	
	else if (!strncmp(Msg->m_pMessage, "/upgr", 5))
	{
		LastChat();
		if (!m_pPlayer->m_AccData.m_UserID) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "你还没有登录！ 输入 /account 获取帮助");

		char supgr[256], andg[64];
		if (sscanf(Msg->m_pMessage, "/upgr %s", supgr) != 1)
		{
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "使用 /upgr <dmg>, <hp>, <handle>, <ammoregen>, <ammo>, <stats> 来给你升级");
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "<dmg> 是攻击力, <hp>是血量, <handle>是攻速(必升 超神), <ammoregen>是子弹恢复速度, <ammo>是子弹总数, <stats>是显示当前全部等级");
			return;
		}
		if (!strcmp(supgr, "handle"))
		{
			if (m_pPlayer->m_AccData.m_Handle >= 300)
				return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "已达到最高等级 (300).");
			if (m_pPlayer->m_AccData.m_Money <= 0)
				return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "升级点不足 (至少1点).");

			m_pPlayer->m_AccData.m_Money--, m_pPlayer->m_AccData.m_Handle++;
			str_format(andg, sizeof(andg), "当前攻速: %d, 当前升级点: %d", m_pPlayer->m_AccData.m_Handle, m_pPlayer->m_AccData.m_Money);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), andg);
			m_pPlayer->m_pAccount->Apply();
			return;
		}
		else if (!strcmp(supgr, "dmg"))
		{
			if (m_pPlayer->m_AccData.m_Dmg >= 21)
				return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "已经达到最高级 (21).");
			if (m_pPlayer->m_AccData.m_Money <= 0)
				return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "升级点不足 (至少1点).");

			m_pPlayer->m_AccData.m_Money--, m_pPlayer->m_AccData.m_Dmg++;
			str_format(andg, sizeof(andg), "当前伤害: %d, 当前升级点: %d", m_pPlayer->m_AccData.m_Dmg, m_pPlayer->m_AccData.m_Money);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), andg);
			m_pPlayer->m_pAccount->Apply();
			return;
		}
		else if (!strcmp(supgr, "hp"))
		{
			if (m_pPlayer->m_AccData.m_Health >= 100)
				return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "已达到最高血量 (100).");
			if (m_pPlayer->m_AccData.m_Money <= 0)
				return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "升级点不足 (至少1点).");

			m_pPlayer->m_AccData.m_Money--, m_pPlayer->m_AccData.m_Health++;
			str_format(andg, sizeof(andg), "当前血量: %d, 最高血量: %d, 当前升级点: %d", m_pPlayer->m_AccData.m_Health, 1000 + m_pPlayer->m_AccData.m_Health * 10 + m_pPlayer->m_AccData.m_Level * 20, m_pPlayer->m_AccData.m_Money);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), andg);
			m_pPlayer->m_pAccount->Apply();
			return;
		}
		else if (!strcmp(supgr, "ammoregen"))
		{
			if (m_pPlayer->m_AccData.m_Ammoregen >= 60)
				return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "子弹恢复已达到最高级 (60).");
			if (m_pPlayer->m_AccData.m_Money <= 0)
				return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "穷逼 (1点都出不起).");

			m_pPlayer->m_AccData.m_Money--, m_pPlayer->m_AccData.m_Ammoregen++;
			str_format(andg, sizeof(andg), "当前子弹恢复等级: %d, 当前升级点: %d", m_pPlayer->m_AccData.m_Ammoregen, m_pPlayer->m_AccData.m_Money);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), andg);
			m_pPlayer->m_pAccount->Apply();
			return;
		}
		else if (!strcmp(supgr, "ammo"))
		{
			if (m_pPlayer->m_AccData.m_Ammo >= 20)
				return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "已达到子弹最高数量 (20).");
			if (m_pPlayer->m_AccData.m_Money < 10)
				return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "升级点不足 (至少10点).");

			m_pPlayer->m_AccData.m_Money -= 10, m_pPlayer->m_AccData.m_Ammo++;
			str_format(andg, sizeof(andg), "当前子弹数: %d, 当前升级点: %d", GameServer()->GetPlayerChar(m_pPlayer->GetCID())->m_mAmmo + m_pPlayer->m_AccData.m_Ammo, m_pPlayer->m_AccData.m_Money);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), andg);
			m_pPlayer->m_pAccount->Apply();
			return;
		}
		else if (!strcmp(supgr, "stats"))
		{
			char aBuf2[32];
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "~~~~~~~~ ! 等级升级明细 ! ~~~~~~~~");
			str_format(aBuf2, sizeof(aBuf2), "伤害: %d", m_pPlayer->m_AccData.m_Dmg);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf2);
			str_format(aBuf2, sizeof(aBuf2), "血量: %d", m_pPlayer->m_AccData.m_Health);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf2);
			str_format(aBuf2, sizeof(aBuf2), "攻速: %d", m_pPlayer->m_AccData.m_Handle);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf2);
			str_format(aBuf2, sizeof(aBuf2), "子弹恢复: %d", m_pPlayer->m_AccData.m_Ammoregen);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf2);
			str_format(aBuf2, sizeof(aBuf2), "子弹: %d", m_pPlayer->m_AccData.m_Ammo);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf2);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "~~~~~~~~ ! 等级升级明细 ! ~~~~~~~~");
			return;
		}
		else return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "这个类别不存在.");
	}
	else if(!strncmp(Msg->m_pMessage, "/turret", 7))
	{
		LastChat();
		if(!m_pPlayer->m_AccData.m_UserID) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "你还没有登录！ 输入 /account 获得账号帮助");

		char supgr[256], andg[64];
		if (sscanf(Msg->m_pMessage, "/turret %s", supgr) != 1)
			return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "提示： /turret info");

		if(!strcmp(supgr, "info"))
		{		
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/turret info - 关于");
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/turret help - 帮助");
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/turret stats - 显示炮塔等级");
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/turret speed - 升级炮塔攻速 (1 炮塔元)");
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/turret dmg - 升级炮塔伤害 (1 炮塔元)");
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/turret ammo - 升级炮塔子弹总数 (1 炮塔元)");
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/turret ammoregen - 升级炮塔子弹恢复速度 (1 炮塔元)");
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/turret range - 升级炮塔发射范围 (1 炮塔元)");
			return;
		}			
		else if(!strcmp(supgr, "help"))
		{
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "使用表情（左Shift）鬼魂防止炮台");
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "炮塔有3种不一样的武器");
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "手枪炮塔: 每秒射击一次敌人  朝任意方向开火（range）");
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "散弹枪炮塔: 每3秒射击一次  朝任意方向放置");
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "激光炮塔: 如果僵尸路过它瞄准的位置，瞬间放置墙挡住（捕捉）僵尸  一个地方放置，走到或跳到另一个无遮挡的地方朝向第一个点放置");
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), 
			return;
		}
		else if(!strcmp(supgr, "dmg"))
		{
			if(m_pPlayer->m_AccData.m_TurretDmg > 100)
				return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "已达到炮塔最高伤害");		
			if(m_pPlayer->m_AccData.m_TurretMoney <= 0)
				return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "你没有足够的炮塔元");
			
			m_pPlayer->m_AccData.m_TurretMoney--, m_pPlayer->m_AccData.m_TurretDmg++;
			str_format(andg, sizeof(andg), "你的炮塔伤害增加了, 现在是: %d", m_pPlayer->m_AccData.m_TurretDmg);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), andg);
			m_pPlayer->m_pAccount->Apply();
			return;
		}			
		else if(!strcmp(supgr, "speed"))
		{
			if(m_pPlayer->m_AccData.m_TurretSpeed >= 500)
				return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "已达到炮塔最高速度");		
			if(m_pPlayer->m_AccData.m_TurretMoney <= 0)
				return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "你个穷逼");
			
			m_pPlayer->m_AccData.m_TurretMoney--, m_pPlayer->m_AccData.m_TurretSpeed++;
			str_format(andg, sizeof(andg), "你的炮塔速度提升了, 现在是: %d", m_pPlayer->m_AccData.m_TurretSpeed);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), andg);
			m_pPlayer->m_pAccount->Apply();
			return;
		}	
		else if (!strcmp(supgr, "ammo"))
		{
			if (m_pPlayer->m_AccData.m_TurretAmmo >= 100)
				return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "已达到炮塔最高子弹数");
			if (m_pPlayer->m_AccData.m_TurretMoney <= 0)
				return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "你没有足够的钱");

			m_pPlayer->m_AccData.m_TurretMoney--, m_pPlayer->m_AccData.m_TurretAmmo++;
			str_format(andg, sizeof(andg), "你的炮塔枪升级了, 现在是: %d", m_pPlayer->m_AccData.m_TurretAmmo);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), andg);
			m_pPlayer->m_pAccount->Apply();
			return;
		}
		else if (!strcmp(supgr, "ammoregen"))
		{
			if (m_pPlayer->m_AccData.m_TurretShotgun >= 75)
				return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "已达到炮塔最高子弹恢复速");
			if (m_pPlayer->m_AccData.m_TurretMoney <= 0)
				return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "你没钱");

			m_pPlayer->m_AccData.m_TurretMoney--, m_pPlayer->m_AccData.m_TurretShotgun++;
			str_format(andg, sizeof(andg), "你的炮塔子弹恢复速升级了, 现在是: %d", m_pPlayer->m_AccData.m_TurretShotgun);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), andg);
			m_pPlayer->m_pAccount->Apply();
			return;
		}
		else if (!strcmp(supgr, "range"))
		{
			if (m_pPlayer->m_AccData.m_TurretRange >= 200)
				return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "已达到炮塔最大攻击范围");
			if (m_pPlayer->m_AccData.m_TurretMoney <= 0)
				return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "没钱滚");

			m_pPlayer->m_AccData.m_TurretMoney--, m_pPlayer->m_AccData.m_TurretRange++;
			str_format(andg, sizeof(andg), "你的炮塔攻击范围提升, 现在是: %d", m_pPlayer->m_AccData.m_TurretRange);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), andg);
			m_pPlayer->m_pAccount->Apply();
			return;
		}
		else if (!strcmp(supgr, "stats"))
		{
			char aBuf[64];
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "------ 炮塔 ------");
			str_format(aBuf, sizeof(aBuf), "等级: %d", m_pPlayer->m_AccData.m_TurretLevel);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
			str_format(aBuf, sizeof(aBuf), "经验: %d", m_pPlayer->m_AccData.m_TurretExp);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
			str_format(aBuf, sizeof(aBuf), "炮塔元: %d", m_pPlayer->m_AccData.m_TurretMoney);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
			str_format(aBuf, sizeof(aBuf), "速度: %d", m_pPlayer->m_AccData.m_TurretSpeed);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
			str_format(aBuf, sizeof(aBuf), "伤害: %d", m_pPlayer->m_AccData.m_TurretDmg);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
			str_format(aBuf, sizeof(aBuf), "子弹: %d", m_pPlayer->m_AccData.m_TurretAmmo);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
			str_format(aBuf, sizeof(aBuf), "子弹恢复: %d", m_pPlayer->m_AccData.m_TurretShotgun);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
			str_format(aBuf, sizeof(aBuf), "范围: %d", m_pPlayer->m_AccData.m_TurretRange);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "------ 炮塔 ------");
			return;
		}
		else
		{
			str_format(andg, sizeof(andg), "没有这个炮塔升级指令: '%s'!", supgr);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), andg);
			return;
		}
	}
	else if (!str_comp_nocase(Msg->m_pMessage, "/me") || !str_comp_nocase(Msg->m_pMessage, "/status") || !str_comp_nocase(Msg->m_pMessage, "/stats"))
	{
		LastChat();
		if (!m_pPlayer->m_AccData.m_UserID) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "你还没有登录！ 输入 /account 获取帮助");

		char aBuf[64];
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "~~~~~~~~ ! 你的账号明细 ! ~~~~~~~~");
		str_format(aBuf, sizeof(aBuf), "登录: %s", m_pPlayer->m_AccData.m_Username);
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
		str_format(aBuf, sizeof(aBuf), "密码: %s", m_pPlayer->m_AccData.m_Password);
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "邮箱: <没有邮箱> (没做)");
		str_format(aBuf, sizeof(aBuf), "等级: %d", m_pPlayer->m_AccData.m_Level);
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
		str_format(aBuf, sizeof(aBuf), "经验: %d/%d", m_pPlayer->m_AccData.m_Exp, m_pPlayer->m_AccData.m_Level);
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
		str_format(aBuf, sizeof(aBuf), "钱数: %d", m_pPlayer->m_AccData.m_Money);
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
		str_format(aBuf, sizeof(aBuf), "冰冻: %s", m_pPlayer->m_AccData.m_Freeze ? "yes" : "no");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "你可以使用这个指令修改密码:");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/password <新密码>");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "~~~~~~~~ ! 你的账户明细 ! ~~~~~~~~");
		return;
	}
	else if(!strncmp(Msg->m_pMessage, "/idlist", 7))
	{
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "------ ID列表 ------");
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(GameServer()->m_apPlayers[i])
			{
				char aBuf[32];
				str_format(aBuf, sizeof(aBuf), "%s ID:%i", GameServer()->Server()->ClientName(i), i);
				GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
			}	
		}
		return;
	}
	else if(!strncmp(Msg->m_pMessage, "/password", 9))
	{
		LastChat();
		if (!m_pPlayer->m_AccData.m_UserID)
			return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "你还没有登录！ 输入 /account 获取帮助");

		char NewPassword[256];
		if(sscanf(Msg->m_pMessage, "/password %s", NewPassword) != 1)
			return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "格式错误 \"/password <新密码>\"");

		m_pPlayer->m_pAccount->NewPassword(NewPassword);
		return;
	}
	else if (!strcmp(Msg->m_pMessage, "/vip"))
	{
		LastChat();
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "--- ViP ---");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "不存在的");
		return;
	}
	else if (!strcmp(Msg->m_pMessage, "/supervip"))
	{
		LastChat();
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "--- 超级ViP ---");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "不存在的");
		return;
	}
	else if (!strcmp(Msg->m_pMessage, "/ultimatevip"))
	{
		LastChat();
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "--- 终极ViP ---");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "不存在的");
		return;
	}
	else if (!strcmp(Msg->m_pMessage, "/info"))
	{
		LastChat();
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "~~~~~~~~ 欢迎来到服务器介绍 ~~~~~~~~");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "管理员: FlowerFell-Sans,管理员QQ:1421709710");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Panic mod by Kurosio, 翻译者: FlowerFell-Sans");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "~~~~~~~~ 欢迎来到服务器介绍 ~~~~~~~~");
		return;
	}
	else if(!strcmp(Msg->m_pMessage, "/help"))
    {
		LastChat();
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "~~~~~~~~ 欢迎来到服务器帮助 ~~~~~~~~");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/account - 关于和账号教程");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/news - 查看服务器最新消息");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/rules - 查看xPanic的规则");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/pm - 给别人发送私信");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/cmdlist - 查看服务器指令列表");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/vip - 获取vip等级");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/supervip - 获取超级vip等级");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/ultimatevip - 获取终极vip等级");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/turret info - 关于炮台");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/levels - 关于等级");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/shop - 分数商店");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "~~~~~~~~ 欢迎来到服务器帮助 ~~~~~~~~");
		return;
    }
	else if (!strcmp(Msg->m_pMessage, "/levels"))
	{
		LastChat();
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "从40级开始：你的散弹枪每10级多发射1颗子弹");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "50级: +10 子弹");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "100级: +10 子弹");
		return;
	}
	else if (!strcmp(Msg->m_pMessage, "/account"))
	{
		LastChat();
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "-------- 账号 --------");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/register <用户名> <密码> - 注册新账号：不能包含中文");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/login <用户名> <密码> - 登录你的账户");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/logout - 退出登录");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/password - 改密码");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "-------- 账号 --------");
		return;
	}
	else if (!strcmp(Msg->m_pMessage, "/shop"))
	{
		LastChat();
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "----- 商店 -----");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/range [10 分数] - 购买大范围僵尸锤");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/heart [20 score] - 在僵尸阵营购买一滴血");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/jump [3 score] - 购买一次多余的跳跃");
		return;
	}
	else if (!strcmp(Msg->m_pMessage, "/news"))
	{
		LastChat();
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "我们有了全中文！");
		return;
	}
	else if (!strcmp(Msg->m_pMessage, "/policehelp"))
	{
		LastChat();
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/kick <id> - 踢出玩家<ID>");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/freeze <id> - 冰冻玩家<ID>");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/idlist - 显示玩家ID");
		return;
	}
	else if(!strcmp(Msg->m_pMessage, "/cmdlist"))
	{
		LastChat();
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "~~~~~~~~ 欢迎查看指令列表 ~~~~~~~~");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/register, /login, /logout - 账号系统");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/rules, /help, /info - 关于");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/stats, /upgr - 升级系统");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/shop - 分数商店");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "/idlist - 显示玩家ID");	
		if(m_pPlayer->m_AccData.m_PlayerState == 1)
		{
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "#这个是警察局");
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "#/policehelp - 警察局帮助");				
		}
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "~~~~~~~~ 欢迎查看指令列表 ~~~~~~~~");
		return;
	}	
	else if(!strcmp(Msg->m_pMessage, "/rules"))
	{
		LastChat();
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "1. 不要刷经验 (冻结账号)");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "2. 不要骂人 (禁言/封禁 1-7天)");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "3. 不要使用机器人 (永封)");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "4. 不要自杀 (跟刷经验一样)");
		return;
	}
	else if (!strcmp(Msg->m_pMessage, "/heart"))
	{
		LastChat();
		if (!m_pPlayer->m_AccData.m_UserID) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "你没有登录! 输入/XXXXX获取账号帮助");
		if (!GameServer()->GetPlayerChar(m_pPlayer->GetCID())) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "只能在活着的时候使用!");
		if (GameServer()->m_World.m_Paused) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "请等待回合结束");
		if(m_pPlayer->GetTeam() != TEAM_RED) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "只能僵尸用");
		if (m_pPlayer->m_Score < 20) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "分数不足 (20).");

		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "完成!");
		m_pPlayer->m_Score -= 20;
		m_pPlayer->m_ActivesLife = false;
		m_pPlayer->m_LifeActives = false;
		new CLifeHearth(&GameServer()->m_World, vec2(0, 0), m_pPlayer->GetCID());
		return;
	}
	else if (!strcmp(Msg->m_pMessage, "/jump"))
	{
		LastChat();
		if (!m_pPlayer->m_AccData.m_UserID) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "你没有登录! 输/account");
		if (!GameServer()->GetPlayerChar(m_pPlayer->GetCID())) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "活着用!");
		if (GameServer()->m_World.m_Paused) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "等回合结束.");
		if (m_pPlayer->m_Score < 3) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "你没分 (3).");

		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "完成!");
		m_pPlayer->m_JumpsShop++;
		m_pPlayer->m_Score -= 3;
		GameServer()->GetPlayerChar(m_pPlayer->GetCID())->m_Core.m_Jumps += 1;
		return;
	}
	else if (!strcmp(Msg->m_pMessage, "/range"))
	{
		LastChat();
		if (!m_pPlayer->m_AccData.m_UserID) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "你没登录");

		if (!GameServer()->GetPlayerChar(m_pPlayer->GetCID())) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "只能活着的时候用!");
		if (GameServer()->m_World.m_Paused) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "等回合结束.");
		if (m_pPlayer->GetTeam() != TEAM_RED) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "只能僵尸用!");
		if (m_pPlayer->m_Score < 10) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "没有足够分数 (10).");

		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "完成!");
		m_pPlayer->m_RangeShop = true;
		m_pPlayer->m_Score -= 10;
		return;
	}
	else if (!strncmp(Msg->m_pMessage, "/prefix", 7) && m_pPlayer->m_AccData.m_UserID && 
		(GameServer()->Server()->IsAuthed(m_pPlayer->GetCID()) || m_pPlayer->m_AccData.m_PlayerState == 2 || m_pPlayer->m_AccData.m_UserID == g_Config.m_SvOwnerAccID))
	{
		LastChat();
		char aBuf[24];
		m_pPlayer->m_Prefix ^= true;
		str_format(aBuf, sizeof(aBuf), "Your Prefix %s", m_pPlayer->m_Prefix ? "enable" : "disable");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
		return;
	}
	else if (!strncmp(Msg->m_pMessage, "/w", 3))
	{
		LastChat();
		int id;
		if (sscanf(Msg->m_pMessage, "/w %d", &id) != 1)
			return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "提示 /w <id> <text>");
		
		int cid2 = clamp(id, 0, (int)MAX_CLIENTS - 1);
		char pLsMsg[128];
		str_copy(pLsMsg, Msg->m_pMessage + 5, 128);
		GameServer()->SendPM(m_pPlayer->GetCID(), cid2, pLsMsg);
		return;
	}
	else if (!strncmp(Msg->m_pMessage, "/setlvl", 7) && GameServer()->Server()->IsAuthed(m_pPlayer->GetCID()))
	{
		LastChat();
		int id, size;
		if ((sscanf(Msg->m_pMessage, "/setlvl %d %d", &id, &size)) != 2)
			return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Please use: /setlvl <id> <level>");

		int cid2 = clamp(id, 0, (int)MAX_CLIENTS - 1);
		if (!GameServer()->m_apPlayers[cid2]) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "There is no such player!'");
		if (!GameServer()->m_apPlayers[cid2]->m_AccData.m_UserID) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "The player is not logged in account!");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "You had to change your level!'"), GameServer()->m_apPlayers[cid2]->m_AccData.m_Level = size, GameServer()->m_apPlayers[cid2]->m_AccData.m_Exp = 0, GameServer()->m_apPlayers[cid2]->m_pAccount->Apply();
		return;
	}
	else if (!strncmp(Msg->m_pMessage, "/setmoney", 9) && GameServer()->Server()->IsAuthed(m_pPlayer->GetCID()))
	{
		LastChat();
		int id, size;
		if ((sscanf(Msg->m_pMessage, "/setmoney %d %d", &id, &size)) != 2)
			return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Please use: /setmoney <id> <money>");

		int cid2 = clamp(id, 0, (int)MAX_CLIENTS - 1);
		if (!GameServer()->m_apPlayers[cid2]) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "There is no such player!'");
		if (!GameServer()->m_apPlayers[cid2]->m_AccData.m_UserID) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "The player is not logged in account!");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "You had to change your level!'"), GameServer()->SendChatTarget(cid2, "Your money changed!'"), GameServer()->m_apPlayers[cid2]->m_AccData.m_Money = size, GameServer()->m_apPlayers[cid2]->m_pAccount->Apply();
		return;
	}
	else if (!strncmp(Msg->m_pMessage, "/freeze", 6) && (m_pPlayer->m_AccData.m_PlayerState == 1 || GameServer()->Server()->IsAuthed(m_pPlayer->GetCID())))
	{
		LastChat();
		int id;
		if(sscanf(Msg->m_pMessage, "/freeze %d", &id) != 1) 
			return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Please use: /freeze <id>");
		
		int cid2 = clamp(id, 0, (int)MAX_CLIENTS - 1);
		if(cid2 == m_pPlayer->GetCID()) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "You can not freeze your account!");
		if (!GameServer()->m_apPlayers[cid2]) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "There is no such player!'");
		if (!GameServer()->m_apPlayers[cid2]->m_AccData.m_UserID) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "The player is not logged in account!");

		GameServer()->m_apPlayers[cid2]->m_AccData.m_Freeze ^= true;
		GameServer()->m_apPlayers[cid2]->m_pAccount->Apply();
		
		char buf[128];
		str_format(buf, sizeof(buf), "You %s account player '%s'", GameServer()->m_apPlayers[cid2]->m_AccData.m_Freeze ? "Freeze":"Unfreeze", GameServer()->Server()->ClientName(cid2));
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), buf);		
		str_format(buf, sizeof(buf), "Your account %s police '%s'", GameServer()->m_apPlayers[cid2]->m_AccData.m_Freeze ? "Freeze":"Unfreeze", GameServer()->Server()->ClientName(m_pPlayer->GetCID()));
		GameServer()->SendChatTarget(cid2, buf);	
		dbg_msg("police", "Police '%s' freeze acc player '%s' login '%s'", GameServer()->Server()->ClientName(m_pPlayer->GetCID()), GameServer()->Server()->ClientName(cid2), GameServer()->m_apPlayers[cid2]->m_AccData.m_Username);			
		return;
	}
	else if (!strncmp(Msg->m_pMessage, "/kick", 5) && m_pPlayer->m_AccData.m_PlayerState == 1)
	{
		LastChat();
		int id;
		if ((sscanf(Msg->m_pMessage, "/kick %d", &id)) != 1)
			return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Please use: /kick <id>");

		int cid2 = clamp(id, 0, (int)MAX_CLIENTS - 1);
		if (!GameServer()->m_apPlayers[cid2]) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "There is no such player!'");

		char buf[64];
		str_format(buf, sizeof(buf), "%s kicked by the Police", GameServer()->Server()->ClientName(cid2));
		GameServer()->SendChatTarget(-1, buf);		
			
		GameServer()->Server()->Kick(cid2, "You was kicked by the Police!");			
		return;
	}
	else if (!strncmp(Msg->m_pMessage, "/setgroup", 9) && GameServer()->Server()->IsAuthed(m_pPlayer->GetCID()))
	{
		LastChat();
		int id, size;
		if (sscanf(Msg->m_pMessage, "/setgroup %d %d", &id, &size) != 2)
		{
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Please use: /setgroup <id> <groupid>"); 
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Group ID: 0 - Removed, 1 - Police, 2 - VIP, 3 - Helper"); return;
		}
		int cid2 = clamp(id, 0, (int)MAX_CLIENTS - 1);
		char gname[4][12] = {"", "police", "vip", "helper"}, aBuf[64];

		if (!GameServer()->m_apPlayers[cid2]) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "There is no such player!");
		if (!GameServer()->m_apPlayers[cid2]->m_AccData.m_UserID) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "The player is not logged in account!");

		switch (size)
		{
			case 0:
				if (GameServer()->m_apPlayers[cid2]->m_AccData.m_PlayerState)
				{
					str_format(aBuf, sizeof(aBuf), "Removed group %s for player '%s'", gname[GameServer()->m_apPlayers[cid2]->m_AccData.m_PlayerState], GameServer()->Server()->ClientName(cid2));
					GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
					str_format(aBuf, sizeof(aBuf), "Your group removed %s!", gname[GameServer()->m_apPlayers[cid2]->m_AccData.m_PlayerState]);
					GameServer()->SendChatTarget(cid2, aBuf);
					GameServer()->m_apPlayers[cid2]->m_AccData.m_PlayerState = 0;
				}
				else GameServer()->SendChatTarget(m_pPlayer->GetCID(), "This player already no in group!"); break;
			default:
				if(size > 3 || size < 0) return GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Group ID not found!");
				GameServer()->m_apPlayers[cid2]->m_AccData.m_PlayerState = size;
				str_format(aBuf, sizeof(aBuf), "Set group %s for player '%s'", gname[GameServer()->m_apPlayers[cid2]->m_AccData.m_PlayerState], GameServer()->Server()->ClientName(cid2));
				GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
				str_format(aBuf, sizeof(aBuf), "Your group set %s!", gname[GameServer()->m_apPlayers[cid2]->m_AccData.m_PlayerState]);
				GameServer()->SendChatTarget(cid2, aBuf); break;
		}
		return;
	}

	if(!strncmp(Msg->m_pMessage, "/", 1))
	{
		LastChat();
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "错误的指令. 输入 /cmdlist");
	}

}

void CCmd::LastChat()
{
	 m_pPlayer->m_LastChat = GameServer()->Server()->Tick();
}
