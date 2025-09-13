# MUD
TCP/IP multi-user dungeon

## Current Status - 09.13.25
As of now, the basic structure of the game server is complete.   
Core functionalities such as chat, movement, and interaction with the world are implemented.   
The server supports multiple users, and basic game actions like speaking, teleporting, and interacting with NPCs or items are functional.


## Playable Features
- **Various Chatting Functions**: Support for multiple types of chat, including:
  - Say
  - Shout
  - Whisper
- **Coordinate Movement**: The ability for players to move to specific coordinates within the game world.
- **Portal Movement**: The ability to travel through portals to different areas or locations.
- **Interaction**: Ability to interact with:
  - NPCs (Non-Playable Characters)
  - Items (e.g., pick up, use, drop)
  - Portals (teleportation)

## Commands
- **Say**: In-game chat command for regular messages.
- **Shout**: A command for sending loud messages that can be heard by all players in the area or on the server.
- **Whisper**: A command for sending private messages to other players.
- **Directional Movement**: Commands for moving in specific directions (e.g., North, South, East, West).
- **Coordinate Movement**: Commands to teleport to specific coordinates within the game world.
- **Map Output**: A command to output the current map or area layout.
- **Interaction**: Commands for interacting with NPCs, objects, and portals.

## Logging
- **Chat Logs**: Logs all player messages including "say", "shout", and "whisper".
- **Connection Logs**: Tracks user connections and disconnections to the server, providing timestamps.
- **Exit Logs**: Logs player exits from the game.
- Logs are written in the following format: [Timestamp] message content -> server.log

## Multi-User Connection Testing Completed
- **Mudlet Connection**: The server successfully handles multiple user connections, and chat functionalities were tested using the Mudlet client.

## Roadmap / Future Plans

### Short-Term Goals
- **Enhance Game Interactions**: Implement additional interaction features such as quests, trading systems, and more dynamic NPC dialogues.
- **Combat System**: Introduce a basic combat system (PvE and PvP), including hit points, skills, and inventory management.
- **Map and Area Expansion**: Add more game areas, maps, and teleportation points for more exploration.
- **Advanced Movement**: Improve movement mechanics with speed and stamina systems.
- **Performance Optimization**: Test and optimize server performance for handling higher numbers of concurrent players.

### Long-Term Plans
- **Advanced Quest System**: Introduce dynamic quests, NPCs with complex behavior, and AI interactions.
- **Combat and Crafting**: Develop a more advanced combat system with a focus on skills and tactics, as well as a crafting system for players to create weapons, armor, and potions.
- **Player Economy**: Add a player-driven economy with shops, item trading, and resource management.
- **Social Features**: Implement social systems like guilds, team-based quests, and ranking.
- **Server Scalability**: Improve server architecture to handle hundreds or thousands of players concurrently in larger worlds.
