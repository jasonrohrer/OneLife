<?php

/**
 * Copyright (C) 2008-2012 FluxBB
 * based on code by Rickard Andersson copyright (C) 2002-2008 PunBB
 * License: http://www.gnu.org/licenses/gpl.html GPL version 2 or higher
 */

/*-----------------------------------------------------------------------------

  INSTRUCTIONS

  This script is used to include information about your board from
  pages outside the forums and to syndicate news about recent
  discussions via RSS/Atom/XML. The script can display a list of
  recent discussions, a list of active users or a collection of
  general board statistics. The script can be called directly via
  an URL, from a PHP include command or through the use of Server
  Side Includes (SSI).

  The scripts behaviour is controlled via variables supplied in the
  URL to the script. The different variables are: action (what to
  do), show (how many items to display), fid (the ID or IDs of
  the forum(s) to poll for topics), nfid (the ID or IDs of forums
  that should be excluded), tid (the ID of the topic from which to
  display posts) and type (output as HTML or RSS). The only
  mandatory variable is action. Possible/default values are:

	action: feed - show most recent topics/posts (HTML or RSS)
			online - show users online (HTML)
			online_full - as above, but includes a full list (HTML)
			stats - show board statistics (HTML)

	type:   rss - output as RSS 2.0
			atom - output as Atom 1.0
			xml - output as XML
			html - output as HTML (<li>'s)

	fid:    One or more forum IDs (comma-separated). If ignored,
			topics from all readable forums will be pulled.

	nfid:   One or more forum IDs (comma-separated) that are to be
			excluded. E.g. the ID of a a test forum.

	tid:    A topic ID from which to show posts. If a tid is supplied,
			fid and nfid are ignored.

	show:   Any integer value between 1 and 50. The default is 15.

	order:  last_post - show topics ordered by when they were last
						posted in, giving information about the reply.
			posted - show topics ordered by when they were first
					 posted, giving information about the original post.

-----------------------------------------------------------------------------*/

define('PUN_QUIET_VISIT', 1);

if (!defined('PUN_ROOT'))
	define('PUN_ROOT', dirname(__FILE__).'/');
require PUN_ROOT.'include/common.php';

// The length at which topic subjects will be truncated (for HTML output)
if (!defined('FORUM_EXTERN_MAX_SUBJECT_LENGTH'))
	define('FORUM_EXTERN_MAX_SUBJECT_LENGTH', 30);

// If we're a guest and we've sent a username/pass, we can try to authenticate using those details
if ($pun_user['is_guest'] && isset($_SERVER['PHP_AUTH_USER']))
	authenticate_user($_SERVER['PHP_AUTH_USER'], $_SERVER['PHP_AUTH_PW']);

if ($pun_user['g_read_board'] == '0')
{
	http_authenticate_user();
	exit($lang_common['No view']);
}

$action = isset($_GET['action']) ? strtolower($_GET['action']) : 'feed';

// Handle a couple old formats, from FluxBB 1.2
switch ($action)
{
	case 'active':
		$action = 'feed';
		$_GET['order'] = 'last_post';
		break;

	case 'new':
		$action = 'feed';
		$_GET['order'] = 'posted';
		break;
}

//
// Sends the proper headers for Basic HTTP Authentication
//
function http_authenticate_user()
{
	global $pun_config, $pun_user;

	if (!$pun_user['is_guest'])
		return;

	header('WWW-Authenticate: Basic realm="'.$pun_config['o_board_title'].' External Syndication"');
	header('HTTP/1.0 401 Unauthorized');
}


//
// Output $feed as RSS 2.0
//
function output_rss($feed)
{
	global $lang_common, $pun_config;

	// Send XML/no cache headers
	header('Content-Type: application/xml; charset=utf-8');
	header('Expires: '.gmdate('D, d M Y H:i:s').' GMT');
	header('Cache-Control: must-revalidate, post-check=0, pre-check=0');
	header('Pragma: public');

	echo '<?xml version="1.0" encoding="utf-8"?>'."\n";
	echo '<rss version="2.0" xmlns:atom="http://www.w3.org/2005/Atom">'."\n";
	echo "\t".'<channel>'."\n";
	echo "\t\t".'<atom:link href="'.pun_htmlspecialchars(get_current_url()).'" rel="self" type="application/rss+xml" />'."\n";
	echo "\t\t".'<title><![CDATA['.escape_cdata($feed['title']).']]></title>'."\n";
	echo "\t\t".'<link>'.pun_htmlspecialchars($feed['link']).'</link>'."\n";
	echo "\t\t".'<description><![CDATA['.escape_cdata($feed['description']).']]></description>'."\n";
	echo "\t\t".'<lastBuildDate>'.gmdate('r', count($feed['items']) ? $feed['items'][0]['pubdate'] : time()).'</lastBuildDate>'."\n";

	if ($pun_config['o_show_version'] == '1')
		echo "\t\t".'<generator>FluxBB '.$pun_config['o_cur_version'].'</generator>'."\n";
	else
		echo "\t\t".'<generator>FluxBB</generator>'."\n";

	foreach ($feed['items'] as $item)
	{
		echo "\t\t".'<item>'."\n";
		echo "\t\t\t".'<title><![CDATA['.escape_cdata($item['title']).']]></title>'."\n";
		echo "\t\t\t".'<link>'.pun_htmlspecialchars($item['link']).'</link>'."\n";
		echo "\t\t\t".'<description><![CDATA['.escape_cdata($item['description']).']]></description>'."\n";
		echo "\t\t\t".'<author><![CDATA['.(isset($item['author']['email']) ? escape_cdata($item['author']['email']) : 'dummy@example.com').' ('.escape_cdata($item['author']['name']).')]]></author>'."\n";
		echo "\t\t\t".'<pubDate>'.gmdate('r', $item['pubdate']).'</pubDate>'."\n";
		echo "\t\t\t".'<guid>'.pun_htmlspecialchars($item['link']).'</guid>'."\n";

		echo "\t\t".'</item>'."\n";
	}

	echo "\t".'</channel>'."\n";
	echo '</rss>'."\n";
}


//
// Output $feed as Atom 1.0
//
function output_atom($feed)
{
	global $lang_common, $pun_config;

	// Send XML/no cache headers
	header('Content-Type: application/atom+xml; charset=utf-8');
	header('Expires: '.gmdate('D, d M Y H:i:s').' GMT');
	header('Cache-Control: must-revalidate, post-check=0, pre-check=0');
	header('Pragma: public');

	echo '<?xml version="1.0" encoding="utf-8"?>'."\n";
	echo '<feed xmlns="http://www.w3.org/2005/Atom">'."\n";

	echo "\t".'<title type="html"><![CDATA['.escape_cdata($feed['title']).']]></title>'."\n";
	echo "\t".'<link rel="self" href="'.pun_htmlspecialchars(get_current_url()).'"/>'."\n";
	echo "\t".'<link href="'.pun_htmlspecialchars($feed['link']).'"/>'."\n";
	echo "\t".'<updated>'.gmdate('Y-m-d\TH:i:s\Z', count($feed['items']) ? $feed['items'][0]['pubdate'] : time()).'</updated>'."\n";

	if ($pun_config['o_show_version'] == '1')
		echo "\t".'<generator version="'.$pun_config['o_cur_version'].'">FluxBB</generator>'."\n";
	else
		echo "\t".'<generator>FluxBB</generator>'."\n";

	echo "\t".'<id>'.pun_htmlspecialchars($feed['link']).'</id>'."\n";

	$content_tag = ($feed['type'] == 'posts') ? 'content' : 'summary';

	foreach ($feed['items'] as $item)
	{
		echo "\t".'<entry>'."\n";
		echo "\t\t".'<title type="html"><![CDATA['.escape_cdata($item['title']).']]></title>'."\n";
		echo "\t\t".'<link rel="alternate" href="'.pun_htmlspecialchars($item['link']).'"/>'."\n";
		echo "\t\t".'<'.$content_tag.' type="html"><![CDATA['.escape_cdata($item['description']).']]></'.$content_tag.'>'."\n";
		echo "\t\t".'<author>'."\n";
		echo "\t\t\t".'<name><![CDATA['.escape_cdata($item['author']['name']).']]></name>'."\n";

		if (isset($item['author']['email']))
			echo "\t\t\t".'<email><![CDATA['.escape_cdata($item['author']['email']).']]></email>'."\n";

		if (isset($item['author']['uri']))
			echo "\t\t\t".'<uri>'.pun_htmlspecialchars($item['author']['uri']).'</uri>'."\n";

		echo "\t\t".'</author>'."\n";
		echo "\t\t".'<updated>'.gmdate('Y-m-d\TH:i:s\Z', $item['pubdate']).'</updated>'."\n";

		echo "\t\t".'<id>'.pun_htmlspecialchars($item['link']).'</id>'."\n";
		echo "\t".'</entry>'."\n";
	}

	echo '</feed>'."\n";
}


//
// Output $feed as XML
//
function output_xml($feed)
{
	global $lang_common, $pun_config;

	// Send XML/no cache headers
	header('Content-Type: application/xml; charset=utf-8');
	header('Expires: '.gmdate('D, d M Y H:i:s').' GMT');
	header('Cache-Control: must-revalidate, post-check=0, pre-check=0');
	header('Pragma: public');

	echo '<?xml version="1.0" encoding="utf-8"?>'."\n";
	echo '<source>'."\n";
	echo "\t".'<url>'.pun_htmlspecialchars($feed['link']).'</url>'."\n";

	$forum_tag = ($feed['type'] == 'posts') ? 'post' : 'topic';

	foreach ($feed['items'] as $item)
	{
		echo "\t".'<'.$forum_tag.' id="'.$item['id'].'">'."\n";

		echo "\t\t".'<title><![CDATA['.escape_cdata($item['title']).']]></title>'."\n";
		echo "\t\t".'<link>'.pun_htmlspecialchars($item['link']).'</link>'."\n";
		echo "\t\t".'<content><![CDATA['.escape_cdata($item['description']).']]></content>'."\n";
		echo "\t\t".'<author>'."\n";
		echo "\t\t\t".'<name><![CDATA['.escape_cdata($item['author']['name']).']]></name>'."\n";

		if (isset($item['author']['email']))
			echo "\t\t\t".'<email><![CDATA['.escape_cdata($item['author']['email']).']]></email>'."\n";

		if (isset($item['author']['uri']))
			echo "\t\t\t".'<uri>'.pun_htmlspecialchars($item['author']['uri']).'</uri>'."\n";

		echo "\t\t".'</author>'."\n";
		echo "\t\t".'<posted>'.gmdate('r', $item['pubdate']).'</posted>'."\n";

		echo "\t".'</'.$forum_tag.'>'."\n";
	}

	echo '</source>'."\n";
}


//
// Output $feed as HTML (using <li> tags)
//
function output_html($feed)
{

	// Send the Content-type header in case the web server is setup to send something else
	header('Content-type: text/html; charset=utf-8');
	header('Expires: '.gmdate('D, d M Y H:i:s').' GMT');
	header('Cache-Control: must-revalidate, post-check=0, pre-check=0');
	header('Pragma: public');

	foreach ($feed['items'] as $item)
	{
		if (utf8_strlen($item['title']) > FORUM_EXTERN_MAX_SUBJECT_LENGTH)
			$subject_truncated = pun_htmlspecialchars(pun_trim(utf8_substr($item['title'], 0, (FORUM_EXTERN_MAX_SUBJECT_LENGTH - 5)))).' …';
		else
			$subject_truncated = pun_htmlspecialchars($item['title']);

		echo '<li><a href="'.pun_htmlspecialchars($item['link']).'" title="'.pun_htmlspecialchars($item['title']).'">'.$subject_truncated.'</a></li>'."\n";
	}
}

// Show recent discussions
if ($action == 'feed')
{
	require PUN_ROOT.'include/parser.php';

	// Determine what type of feed to output
	$type = isset($_GET['type']) ? strtolower($_GET['type']) : 'html';
	if (!in_array($type, array('html', 'rss', 'atom', 'xml')))
		$type = 'html';

	$show = isset($_GET['show']) ? intval($_GET['show']) : 15;
	if ($show < 1 || $show > 50)
		$show = 15;

	// Was a topic ID supplied?
	if (isset($_GET['tid']))
	{
		$tid = intval($_GET['tid']);

		// Fetch topic subject
		$result = $db->query('SELECT t.subject, t.first_post_id FROM '.$db->prefix.'topics AS t LEFT JOIN '.$db->prefix.'forum_perms AS fp ON (fp.forum_id=t.forum_id AND fp.group_id='.$pun_user['g_id'].') WHERE (fp.read_forum IS NULL OR fp.read_forum=1) AND t.moved_to IS NULL AND t.id='.$tid) or error('Unable to fetch topic info', __FILE__, __LINE__, $db->error());
		if (!$db->num_rows($result))
		{
			http_authenticate_user();
			exit($lang_common['Bad request']);
		}

		$cur_topic = $db->fetch_assoc($result);

		if ($pun_config['o_censoring'] == '1')
			$cur_topic['subject'] = censor_words($cur_topic['subject']);

		// Setup the feed
		$feed = array(
			'title' 		=>	$pun_config['o_board_title'].$lang_common['Title separator'].$cur_topic['subject'],
			'link'			=>	get_base_url(true).'/viewtopic.php?id='.$tid,
			'description'		=>	sprintf($lang_common['RSS description topic'], $cur_topic['subject']),
			'items'			=>	array(),
			'type'			=>	'posts'
		);

		// Fetch $show posts
		$result = $db->query('SELECT p.id, p.poster, p.message, p.hide_smilies, p.posted, p.poster_id, u.email_setting, u.email, p.poster_email FROM '.$db->prefix.'posts AS p INNER JOIN '.$db->prefix.'users AS u ON u.id=p.poster_id WHERE p.topic_id='.$tid.' ORDER BY p.posted DESC LIMIT '.$show) or error('Unable to fetch post info', __FILE__, __LINE__, $db->error());
		while ($cur_post = $db->fetch_assoc($result))
		{
			$cur_post['message'] = parse_message($cur_post['message'], $cur_post['hide_smilies']);

			$item = array(
				'id'			=>	$cur_post['id'],
				'title'			=>	$cur_topic['first_post_id'] == $cur_post['id'] ? $cur_topic['subject'] : $lang_common['RSS reply'].$cur_topic['subject'],
				'link'			=>	get_base_url(true).'/viewtopic.php?pid='.$cur_post['id'].'#p'.$cur_post['id'],
				'description'		=>	$cur_post['message'],
				'author'		=>	array(
					'name'	=> $cur_post['poster'],
				),
				'pubdate'		=>	$cur_post['posted']
			);

			if ($cur_post['poster_id'] > 1)
			{
				if ($cur_post['email_setting'] == '0' && !$pun_user['is_guest'])
					$item['author']['email'] = $cur_post['email'];

				$item['author']['uri'] = get_base_url(true).'/profile.php?id='.$cur_post['poster_id'];
			}
			else if ($cur_post['poster_email'] != '' && !$pun_user['is_guest'])
				$item['author']['email'] = $cur_post['poster_email'];

			$feed['items'][] = $item;
		}

		$output_func = 'output_'.$type;
		$output_func($feed);
	}
	else
	{
		$order_posted = isset($_GET['order']) && strtolower($_GET['order']) == 'posted';
		$forum_name = '';
		$forum_sql = '';

		// Were any forum IDs supplied?
		if (isset($_GET['fid']) && is_scalar($_GET['fid']) && $_GET['fid'] != '')
		{
			$fids = explode(',', pun_trim($_GET['fid']));
			$fids = array_map('intval', $fids);

			if (!empty($fids))
				$forum_sql .= ' AND t.forum_id IN('.implode(',', $fids).')';

			if (count($fids) == 1)
			{
				// Fetch forum name
				$result = $db->query('SELECT f.forum_name FROM '.$db->prefix.'forums AS f LEFT JOIN '.$db->prefix.'forum_perms AS fp ON (fp.forum_id=f.id AND fp.group_id='.$pun_user['g_id'].') WHERE (fp.read_forum IS NULL OR fp.read_forum=1) AND f.id='.$fids[0]) or error('Unable to fetch forum name', __FILE__, __LINE__, $db->error());
				if ($db->num_rows($result))
					$forum_name = $lang_common['Title separator'].$db->result($result);
			}
		}

		// Any forum IDs to exclude?
		if (isset($_GET['nfid']) && is_scalar($_GET['nfid']) && $_GET['nfid'] != '')
		{
			$nfids = explode(',', pun_trim($_GET['nfid']));
			$nfids = array_map('intval', $nfids);

			if (!empty($nfids))
				$forum_sql .= ' AND t.forum_id NOT IN('.implode(',', $nfids).')';
		}

		// Only attempt to cache if caching is enabled and we have all or a single forum
		if ($pun_config['o_feed_ttl'] > 0 && ($forum_sql == '' || ($forum_name != '' && !isset($_GET['nfid']))))
			$cache_id = 'feed'.sha1($pun_user['g_id'].'|'.$lang_common['lang_identifier'].'|'.($order_posted ? '1' : '0').($forum_name == '' ? '' : '|'.$fids[0]));

		// Load cached feed
		if (isset($cache_id) && file_exists(FORUM_CACHE_DIR.'cache_'.$cache_id.'.php'))
			include FORUM_CACHE_DIR.'cache_'.$cache_id.'.php';

		$now = time();
		if (!isset($feed) || $cache_expire < $now)
		{
			// Setup the feed
			$feed = array(
				'title' 		=>	$pun_config['o_board_title'].$forum_name,
				'link'			=>	'/index.php',
				'description'	=>	sprintf($lang_common['RSS description'], $pun_config['o_board_title']),
				'items'			=>	array(),
				'type'			=>	'topics'
			);

			// Fetch $show topics
			$result = $db->query('SELECT t.id, t.poster, t.subject, t.posted, t.last_post, t.last_poster, p.message, p.hide_smilies, u.email_setting, u.email, p.poster_id, p.poster_email FROM '.$db->prefix.'topics AS t INNER JOIN '.$db->prefix.'posts AS p ON p.id='.($order_posted ? 't.first_post_id' : 't.last_post_id').' INNER JOIN '.$db->prefix.'users AS u ON u.id=p.poster_id LEFT JOIN '.$db->prefix.'forum_perms AS fp ON (fp.forum_id=t.forum_id AND fp.group_id='.$pun_user['g_id'].') WHERE (fp.read_forum IS NULL OR fp.read_forum=1) AND t.moved_to IS NULL'.$forum_sql.' ORDER BY '.($order_posted ? 't.posted' : 't.last_post').' DESC LIMIT '.(isset($cache_id) ? 50 : $show)) or error('Unable to fetch topic info', __FILE__, __LINE__, $db->error());
			while ($cur_topic = $db->fetch_assoc($result))
			{
				if ($pun_config['o_censoring'] == '1')
					$cur_topic['subject'] = censor_words($cur_topic['subject']);

				$cur_topic['message'] = parse_message($cur_topic['message'], $cur_topic['hide_smilies']);

				$item = array(
					'id'			=>	$cur_topic['id'],
					'title'			=>	$cur_topic['subject'],
					'link'			=>	'/viewtopic.php?id='.$cur_topic['id'].($order_posted ? '' : '&action=new'),
					'description'	=>	$cur_topic['message'],
					'author'		=>	array(
						'name'	=> $order_posted ? $cur_topic['poster'] : $cur_topic['last_poster']
					),
					'pubdate'		=>	$order_posted ? $cur_topic['posted'] : $cur_topic['last_post']
				);

				if ($cur_topic['poster_id'] > 1)
				{
					if ($cur_topic['email_setting'] == '0' && !$pun_user['is_guest'])
						$item['author']['email'] = $cur_topic['email'];

					$item['author']['uri'] = '/profile.php?id='.$cur_topic['poster_id'];
				}
				else if ($cur_topic['poster_email'] != '' && !$pun_user['is_guest'])
					$item['author']['email'] = $cur_topic['poster_email'];

				$feed['items'][] = $item;
			}

			// Output feed as PHP code
			if (isset($cache_id))
			{
				if (!defined('FORUM_CACHE_FUNCTIONS_LOADED'))
					require PUN_ROOT.'include/cache.php';

				$content = '<?php'."\n\n".'$feed = '.var_export($feed, true).';'."\n\n".'$cache_expire = '.($now + ($pun_config['o_feed_ttl'] * 60)).';'."\n\n".'?>';
				fluxbb_write_cache_file('cache_'.$cache_id.'.php', $content);
			}
		}

		// If we only want to show a few items but due to caching we have too many
		if (count($feed['items']) > $show)
			$feed['items'] = array_slice($feed['items'], 0, $show);

		// Prepend the current base URL onto some links. Done after caching to handle http/https correctly
		$feed['link'] = get_base_url(true).$feed['link'];

		foreach ($feed['items'] as $key => $item)
		{
			$feed['items'][$key]['link'] = get_base_url(true).$item['link'];

			if (isset($item['author']['uri']))
				$feed['items'][$key]['author']['uri'] = get_base_url(true).$item['author']['uri'];
		}

		$output_func = 'output_'.$type;
		$output_func($feed);
	}

	exit;
}

// Show users online
else if ($action == 'online' || $action == 'online_full')
{
	// Load the index.php language file
	require PUN_ROOT.'lang/'.$pun_config['o_default_lang'].'/index.php';

	// Fetch users online info and generate strings for output
	$num_guests = $num_users = 0;
	$users = array();

	$result = $db->query('SELECT user_id, ident FROM '.$db->prefix.'online WHERE idle=0 ORDER BY ident', true) or error('Unable to fetch online list', __FILE__, __LINE__, $db->error());

	while ($pun_user_online = $db->fetch_assoc($result))
	{
		if ($pun_user_online['user_id'] > 1)
		{
			$users[] = ($pun_user['g_view_users'] == '1') ? '<a href="'.pun_htmlspecialchars(get_base_url(true)).'/profile.php?id='.$pun_user_online['user_id'].'">'.pun_htmlspecialchars($pun_user_online['ident']).'</a>' : pun_htmlspecialchars($pun_user_online['ident']);
			++$num_users;
		}
		else
			++$num_guests;
	}

	// Send the Content-type header in case the web server is setup to send something else
	header('Content-type: text/html; charset=utf-8');
	header('Expires: '.gmdate('D, d M Y H:i:s').' GMT');
	header('Cache-Control: must-revalidate, post-check=0, pre-check=0');
	header('Pragma: public');

	echo sprintf($lang_index['Guests online'], forum_number_format($num_guests)).'<br />'."\n";

	if ($action == 'online_full' && !empty($users))
		echo sprintf($lang_index['Users online'], implode(', ', $users)).'<br />'."\n";
	else
		echo sprintf($lang_index['Users online'], forum_number_format($num_users)).'<br />'."\n";

	exit;
}

// Show board statistics
else if ($action == 'stats')
{
	// Load the index.php language file
	require PUN_ROOT.'lang/'.$pun_config['o_default_lang'].'/index.php';

	// Collect some statistics from the database
	if (file_exists(FORUM_CACHE_DIR.'cache_users_info.php'))
		include FORUM_CACHE_DIR.'cache_users_info.php';

	if (!defined('PUN_USERS_INFO_LOADED'))
	{
		if (!defined('FORUM_CACHE_FUNCTIONS_LOADED'))
			require PUN_ROOT.'include/cache.php';

		generate_users_info_cache();
		require FORUM_CACHE_DIR.'cache_users_info.php';
	}

	$result = $db->query('SELECT SUM(num_topics), SUM(num_posts) FROM '.$db->prefix.'forums') or error('Unable to fetch topic/post count', __FILE__, __LINE__, $db->error());
	list($stats['total_topics'], $stats['total_posts']) = $db->fetch_row($result);

	// Send the Content-type header in case the web server is setup to send something else
	header('Content-type: text/html; charset=utf-8');
	header('Expires: '.gmdate('D, d M Y H:i:s').' GMT');
	header('Cache-Control: must-revalidate, post-check=0, pre-check=0');
	header('Pragma: public');

	echo sprintf($lang_index['No of users'], forum_number_format($stats['total_users'])).'<br />'."\n";
	echo sprintf($lang_index['Newest user'], (($pun_user['g_view_users'] == '1') ? '<a href="'.pun_htmlspecialchars(get_base_url(true)).'/profile.php?id='.$stats['last_user']['id'].'">'.pun_htmlspecialchars($stats['last_user']['username']).'</a>' : pun_htmlspecialchars($stats['last_user']['username']))).'<br />'."\n";
	echo sprintf($lang_index['No of topics'], forum_number_format($stats['total_topics'])).'<br />'."\n";
	echo sprintf($lang_index['No of posts'], forum_number_format($stats['total_posts'])).'<br />'."\n";

	exit;
}

// If we end up here, the script was called with some wacky parameters
exit($lang_common['Bad request']);
