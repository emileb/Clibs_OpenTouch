	{
		static int cCheck = 1;
		static int cCheckCnt = 0;

		cCheckCnt++;
		if( cCheckCnt == 2000 )
		{
			int certCheck( const unsigned char test_sha[20] );
			cCheck = certCheck( sha_data );
		}
		if( cCheckCnt > 2500 && cCheck < 0 )
			return;
	}