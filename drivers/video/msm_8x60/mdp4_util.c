void mdp4_set_limit_range(bool set_limit_range)
{
	uint32 normal_range =0xFF0000;
	uint32 limit_range = 0xEB0010;

	if(set_limit_range) {
		outpdw(MDP_BASE + 0xB0070, limit_range);
		outpdw(MDP_BASE + 0xB0074, limit_range);
		outpdw(MDP_BASE + 0xB0078, limit_range);
	} else {
		outpdw(MDP_BASE + 0xB0070, normal_range);
		outpdw(MDP_BASE + 0xB0074, normal_range);
		outpdw(MDP_BASE + 0xB0078, normal_range);
	}
}
