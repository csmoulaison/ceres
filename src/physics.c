/*

	DETECTING COLLISIONS

	AABB/Circle colliisons are what we care about at the moment. Our initial idea 
	is to first test circle radius against corners, then test against sides as if
	the circle was an AABB.

	There are three scenarios to cover, as far as I can tell:

		XXX
		XXX         Testing against corners works here.
		XXX
			O

		XXX
		XXX O       Testing against sides works here.
		XXX

		XXX
		X           Testing against sides works here.
		X O

	So it seems like this idea will work, barring some unconsidered edge case.


	RESOLVING COLLISIONS

	Our first idea is to bounce the ship off the walls proportionally to its speed 
	and accounting for penetration distance, with some energy lost on collision.

	In the side case, we flip the velocity axis which is perpendicular to the 
	collision, leaving the other unchanged, aside from energy loss.

	In outside corner case where our velocity is directly inverse to the corner
	normal, we flip the velocity on both axes.

	We need a continuity between these two cases, because there is a distinct 
	threshold in the way you hit a corner which would suddenly flip from corner
	testing to AABB testing, and so we need to attenuate the flippedness each axis
	by how perpendicular we are to the matching side.

	We iteratively solve these collisions so that cases like inside corners are 
	handled. Iterate as many times as it takes (up to some maximum) to not be 
	inside something. In the case of multiple collisions resulting from a given 
	movement, as in a close inside corner case, we choose whichever collision 
	happens first and iterate from there, which might result in a double bounce 
	happening internally within the same simulation frame.

*/

void physics_resolve_velocities(GameState* game) {
	f32 radius = 0.5f;
	for(i32 i = 0; i < 2; i++) {
		GamePlayer* player = &game->players[i];

		// Resolve against level, reducing checks to a 3x3 region around the player
		for(i32 j = 0; j < 25; j++) {
			f32 x = player->position.x + (j % 5) - 1.0f;
			f32 y = player->position.y + (j / 5) - 1.0f;

			// NOW: remove magic 64 for level grid length
			i32 cube_index = (i32)y * 64 + (i32)x;
			//printf("%u ", game->level[cube_index]);
			//if(j % 5 == 4) printf("\n");
			if(game->level[cube_index] == 0) continue;

			//v2 cube_pos = v2_new((i32)x - 0.5f, (i32)y - 0.5f);
			//v2 nearest = v2_max(cube_pos, v2_min(player->position, v2_add(cube_pos, v2_new(1.0f, 1.0f))));
			//v2 delta = v2_sub(player->position, nearest);

			//if(v2_dot(player->velocity, delta) < 0.0f) {
			//	f32 tan_velocity = v2_dot(v2_normalize(delta), player->velocity) * 2.0f;
			//	player->velocity = v2_sub(player->velocity, v2_new(tan_velocity, tan_velocity));
			//}

			//f32 penetration_depth = radius - v2_magnitude(delta);
			//v2 penetration_vector = v2_scale(v2_normalize(delta), penetration_depth);
			//printf("pen vector %f %f\n", penetration_vector.x, penetration_vector.y);
			//player->position = v2_sub(player->position, penetration_vector);

			v2 relative_center = v2_sub(player->position, v2_new((i32)x, (i32)y));
			v2 offset_from_corner = v2_sub(v2_abs(relative_center), v2_new(0.5f, 0.5f));
			f32 penetration = f32_min(f32_max(offset_from_corner.x, offset_from_corner.y), 0.0f)
				+ v2_magnitude(v2_max(offset_from_corner, v2_zero()))
				- radius;

			v2 contact_normal;
			if(offset_from_corner.x > offset_from_corner.y) {
				contact_normal = v2_new(-1.0f, 1.0f);
			}
			if(offset_from_corner.y > offset_from_corner.x) {
				contact_normal = v2_new(1.0f, -1.0f);
			}

			printf("penetration %f\n", penetration);

			if(penetration < 0.0f) {
				player->position = v2_add(player->position, v2_scale(v2_normalize(player->velocity), penetration * 1.2f));
				player->velocity = v2_mult(player->velocity, contact_normal);
				player->velocity = v2_scale(player->velocity, 0.7f);
				player->position = v2_add(player->position, v2_scale(v2_normalize(player->velocity), -penetration));
				//player->velocity = v2_add(player->velocity, v2_scale(contact_normal, v2_magnitude(player->velocity)));
			}
		}

		// Resolve against other players
		for(i32 j = i + 1; j < 2; j++) {
			GamePlayer* other = &game->players[j];
			if(v2_distance(player->position, other->position) < radius * 2.0f) {
				// This is the case of both ships having equal mass.
				//v2 collision_normal = v2_normalize(v2_sub(player->position, other->position));
				v2 vel_diff = v2_sub(player->velocity, other->velocity);
				v2 mass_diff = v2_zero();
				v2 vel_mass_other = v2_scale(v2_mult(v2_identity(), other->velocity), 2.0f);
				v2 mass_total = v2_add(v2_identity(), v2_identity());

				player->velocity = v2_scale(v2_add(v2_mult(player->velocity, mass_diff), v2_div(vel_mass_other, mass_total)), 0.66f);
				other->velocity  = v2_scale(v2_add(vel_diff, player->velocity), 0.66f);

				// Move circles out of each other
				v2 midpoint = v2_scale(v2_add(player->position, other->position), 0.5f);
				f32 distance = v2_distance(player->position, other->position);
				{
					v2 pos_diff = v2_scale(v2_sub(player->position, other->position), radius);
					player->position = v2_add(midpoint, v2_scale(pos_diff, 1.0f / distance));
				}
				{
					v2 pos_diff = v2_scale(v2_sub(other->position, player->position), radius);
					other->position = v2_add(midpoint, v2_scale(pos_diff, 1.0f / distance));
				}
			}
		}
	}
}
