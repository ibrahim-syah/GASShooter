// Copyright 2020 Dan Kestranek.

using UnrealBuildTool;
using System.Collections.Generic;
using System;
using System.IO;
using EpicGames.Core;
using UnrealBuildBase;
using Microsoft.Extensions.Logging;

public class GASShooterTarget : TargetRules
{
	public GASShooterTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V4;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.AddRange( new string[] { "GASShooter" } );

        if (this.bBuildEditor)
        {
            ExtraModuleNames.AddRange(
                new string[]
                {
                    "GASShooterEditor"
                });
        }
    }
}
