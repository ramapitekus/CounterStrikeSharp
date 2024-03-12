using CounterStrikeSharp.API;
using CounterStrikeSharp.API.Core;
using CounterStrikeSharp.API.Modules.Memory;
using CounterStrikeSharp.API.Core.Attributes;
using CounterStrikeSharp.API.Core.Attributes.Registration;
using CounterStrikeSharp.API.Modules.Commands;
using CounterStrikeSharp.API.Modules.Admin;
using CounterStrikeSharp.API.Modules.Utils;
using CounterStrikeSharp.API.Modules.Entities;
using Microsoft.Extensions.Logging.Abstractions;

using CounterStrikeSharp.API.Modules.Menu;
using CounterStrikeSharp.API.Modules.Timers;
using System.Security.Principal;

namespace ShowPlayersInfo;

[MinimumApiVersion(65)]
public class ShowPlayersInfo : BasePlugin
{
    public class UndiscoveredCorpse {
    public string Victim { get; private set; }
    public string Attacker { get; private set; }
    public Vector? Location { get; private set; }
    public string Role { get; private set; }

        public UndiscoveredCorpse(string victim, string attacker, Vector? location, string role)
        {
            Victim = victim;
            Attacker = attacker;
            Location = location;
            Role = role;
        }
}
    public override string ModuleName => "ShowPlayersInfo";

    public override string ModuleVersion => "v1.3.0";
    private Dictionary<IntPtr, TTTPlayer> TTTPlayers = new();
    // treba pridat informaciu o hracovi/meno + kto ho zabil
    private List<UndiscoveredCorpse> UndiscoveredCorpses = new();
    public override string ModuleAuthor => "jockii (ch1nazes)";


    [GameEventHandler]
    public HookResult OnHurt(EventPlayerHurt @event, GameEventInfo info)
    {
        string weap = @event.Weapon;
        var player = @event.Userid;
        // @event.Userid.Pawn.Value.Glow.Glowing = true;
        // @event.Userid.Pawn.Value.Glow.GlowType = 3;
        // @event.Userid.Pawn.Value.Glow.GlowColor.X = 0;
        // @event.Userid.Pawn.Value.Glow.GlowColor.Y = 255;
        // @event.Userid.Pawn.Value.Glow.GlowColor.Z = 0;
        @event.Userid.Pawn.Value.Glow.GlowTeam = 2;
        // @event.Userid.Pawn.Value.Glow.GlowTime = 100;
        @event.Userid.PlayerPawn.Value.DetectedByEnemySensorTime = 86400;
        // @event.Attacker.PlayerPawn.Value.DetectedByEnemySensorTime = 86400;
        @event.Userid.Pawn.Value.Glow.GlowTeam
        // @event.Userid.Pawn.Value.Glow.GlowColorOverride = System.Drawing.Color.FromName("SlateBlue");
        Console.WriteLine(@$"
            GlowType: {player.Pawn.Value.Glow.GlowType}, 
            GlowColor: {player.Pawn.Value.Glow.GlowColor}, 
            GlowTime: {player.Pawn.Value.Glow.GlowTime}, 
            GlowTeam: {player.Pawn.Value.Glow.GlowTeam},
            Flashing: {player.Pawn.Value.Glow.Flashing},
            Glowing: {player.Pawn.Value.Glow.Glowing},
            GlowType: {player.Pawn.Value.Glow.GlowType},
            GlowStartTime: {player.Pawn.Value.Glow.GlowStartTime},
            GlowRangeMin: {player.Pawn.Value.Glow.GlowRangeMin},
        ");
        CCSPlayerController victim = @event.Userid;
        if (weap == "taser") {
            @event.Userid.PlayerPawn.Value.Health += @event.DmgHealth;
            VirtualFunctions.ClientPrint(@event.Attacker.Handle, HudDestination.Alert, $"{victim.PlayerName} is {GetTTTPlayer(victim).role}", 0, 0, 0, 0);
        }
        return HookResult.Continue;
    }

    [GameEventHandler(HookMode.Pre)]
    public HookResult OnPlayerKilled(EventPlayerDeath @event, GameEventInfo info){
        CCSPlayerController attacker = @event.Attacker;
        CCSPlayerController victim =  @event.Userid;

        // @event.Attacker = @event.Userid; // Simulate suicide

        Vector? orig = @event.Userid.PlayerPawn.Value.AbsOrigin;
        // need to create a copy not a reference to the original origin, as this changes and might become null if player died later
        Vector playerKilledLocation = new Vector(orig.X, orig.Y, orig.Z);
        // attacker.Pawn.Value.Glow.Glowing = true;
        Console.WriteLine(victim.Pawn.Value.Glow.Glowing);

        UndiscoveredCorpses.Add(new UndiscoveredCorpse(victim.PlayerName, attacker.PlayerName, playerKilledLocation, GetTTTPlayer(victim).role));
        return HookResult.Stop;
    }

    public override void Load(bool hotReload)
    {
        RegisterListener<Listeners.OnEntitySpawned>(entity =>
        {
            if (entity.DesignerName != "smokegrenade_projectile") return;

            var projectile = new CSmokeGrenadeProjectile(entity.Handle);

            // Changes smoke grenade colour to a random colour each time.
            Server.NextFrame(() =>
            {
                projectile.SmokeColor.X = Random.Shared.NextSingle() * 255.0f;
                projectile.SmokeColor.Y = Random.Shared.NextSingle() * 255.0f;
                projectile.SmokeColor.Z = Random.Shared.NextSingle() * 255.0f;
            });
        });
    }

    [GameEventHandler(HookMode.Pre)]
    public HookResult DontEndRound(EventRoundEnd @event, GameEventInfo info){
        @event.Nomusic = 1;
        Console.WriteLine("kolo skoncilo");
        return HookResult.Stop;
    }

    public List<TTTPlayer> traitors = new();

    [GameEventHandler]
    public HookResult OnEventRoundStart(EventRoundPrestart @event, GameEventInfo info){
        var playerEntities = Utilities.FindAllEntitiesByDesignerName<CCSPlayerController>("cs_player_controller").ToList();
        var nrPlayers = playerEntities.Count;
        var nrTraitors = Convert.ToInt16(Math.Round(nrPlayers * 1.0));
        var nrDetectives = Convert.ToInt16(Math.Round(nrPlayers * 0.0));

        
        // Shuffle the player list
        var random = new Random();
        var shuffledPlayers = playerEntities.OrderBy(x => random.Next()).ToList();

        for (int i = 0; i < shuffledPlayers.Count; i++)
        {
            var player = shuffledPlayers[i];
            if (!player.IsValid || !player.PawnIsAlive) continue;

            var tttPlayer = new TTTPlayer(player);

            if (i < nrTraitors)
            {
                traitors.Add(tttPlayer);
                tttPlayer.role = "Traitor";
            }
            else if (i < nrTraitors + nrDetectives)
            {
                tttPlayer.role = "Detective";
                player.GiveNamedItem("weapon_taser");
                // player.SwitchTeam(CsTeam.CounterTerrorist);
            }
            else
            {
                tttPlayer.role = "Innocent";
            }
            SetTTTPlayer(player, tttPlayer);
        }
        // SetVisibilityForTraitors(playerEntities);
        AddTimer(0.25f, StatusUpdate, TimerFlags.REPEAT | TimerFlags.STOP_ON_MAPCHANGE);
        AddTimer(2.0f, LocationUpdate, TimerFlags.REPEAT | TimerFlags.STOP_ON_MAPCHANGE);

        return HookResult.Continue;
}
    // private void SetVisibilityForTraitors(List<CCSPlayerController>? playerEntities){
    //     foreach (var traitor in traitors){


    //     }
    // }
    private void StatusUpdate(){
        var playerEntities = Utilities.FindAllEntitiesByDesignerName<CCSPlayerController>("cs_player_controller");
        foreach (var player in playerEntities)
        {
            var tttPlayer = GetTTTPlayer(player);
            player.PrintToCenter($"Role: {tttPlayer.role}"); 
        }
    }

    private void LocationUpdate(){
        var playerEntities = Utilities.FindAllEntitiesByDesignerName<CCSPlayerController>("cs_player_controller");
        foreach (var player in playerEntities)
        {
            if (player.PawnIsAlive){
            Vector? playerLocation = player.PlayerPawn.Value.AbsOrigin;
            if (playerLocation != null) {
                UndiscoveredCorpses.RemoveAll( corpse => {
                    float distance = (float)Math.Sqrt(Math.Pow(corpse.Location.X - playerLocation.X, 2) + Math.Pow(corpse.Location.Y - playerLocation.Y, 2) + Math.Pow(corpse.Location.Z - playerLocation.Z, 2));
                    // Server.PrintToChatAll($"Distance from {player.PlayerName} is {distance}");
                    bool shouldRemove = distance <= 100;
                    if (shouldRemove) {
                        // perhaps add a check which prevents the killer to discover the body
                        Server.PrintToChatAll($"Body discovered by {player.PlayerName} - they were {corpse.Role}");
                        return true;
                    }
                    return false;
                }
            );
            }
            }
        }
    }

    public TTTPlayer GetTTTPlayer(CCSPlayerController player)
    {
        TTTPlayers.TryGetValue(player.Handle, out var tttPlayer);
        if (tttPlayer == null)
        {
            if (!player.IsValid)
                return null;
            }
        return TTTPlayers[player.Handle];
    }

    public void SetTTTPlayer(CCSPlayerController player, TTTPlayer tttPlayer)
    {
        TTTPlayers[player.Handle] = tttPlayer;
    }


    public class TTTPlayer
    {
        public CCSPlayerController GetPlayer() => Player;
        public CCSPlayerController Player { get; init; }
        public string role = "kokotko";
        public TTTPlayer(CCSPlayerController player)
        {
            Player = player;
        }
    }
}