<html>
  <head>
  <title>{% block toptitle %}Search Wikipedia with strus{% end %}</title>
  <link href="static/strus.css" rel="stylesheet" type="text/css" />
  <meta http-equiv="content-type" content="text/html; charset=utf-8" />
  </head>
  <body>
  <h1>{% block title %}Project Strus: A demo search engine for Wikipedia (english){% end %}</h1>
  <div id="search_form">
  <div id="search_elements">
  <div id="search_logo">
   <a target="_blank" href="http://project-strus.net">
    <img style="display:block;" width="100%" src="static/strus_logo.jpg" alt="strus logo">
  <!-- Copyright: <a href='http://www.123rf.com/profile_guarding123'>guarding123 / 123RF Stock Photo</a>
  -->
   </a>
  </div>
  <form name="search" class method="GET" action="query">
  <input id="search_input" class="textinput" type="text" maxlength="256" size="32" name="q" tabindex="1" value="{{querystr}}"/>
  <input type="hidden" name="i" value="{{firstrank}}"/>
  <input type="hidden" name="n" value="{{nofranks}}"/>
  <input type="hidden" name="s" value="{{scheme}}"/>
  <input type="radio" name="scheme" value="BM25_dpfc" {% if scheme == "BM25_dpfc" %}checked{% end %}/>BM25_dpfc
  <input type="radio" name="scheme" value="BM25"      {% if scheme == "BM25" %}checked{% end %}/>BM25
  <input id="search_button" type="image" src="static/search_button.jpg" tabindex="2"/>
  </form>
  </div>
  </div>
  {% block ranklist %}
  <p>query answering time: {{exectime}}</p>
  {% for result in results %}
	{% set content = "" %}
	{% set title = "" %}
	{% for attribute in result.attributes() %}
		{% if attribute.name() == 'CONTENT' %}
			{% if content != "" %}
			{%	set content += ' ... ' %}
			{% end %}
			{% set content += attribute.value() %}
		{% elif attribute.name() == 'TITLE' %}
			{% set title += attribute.value() %}
		{% end %}
	{% end %}
	{% set link = title.replace( " ", "_") %}
	<div id="search_rank">
	<div id="rank_docno">{{result.docno()}}</div>
	<div id="rank_weight">{{ "%.4f" % result.weight()}}</div>
	<div id="rank_content">
	<div id="rank_title"><a href="http://en.wikipedia.org/wiki/{{link}}">{{title}}</a></div>
	<div id="rank_summary">{% raw content %}</div>
	</div>
	</div>
  {% end %}
  </div>
  <div id="navigation_form">
  <div id="navigation_elements">
  {% if firstrank > 0 %}
	{% if firstrank > nofranks %}
		{% set nextrank = firstrank - nofranks %}
	{% else %}
		{% set nextrank = 0 %}
	{% end %}
	<form name="nav_prev" class method="GET" action="query">
	<input type="hidden" name="q" value="{{querystr}}"/>
	<input type="hidden" name="n" value="{{nofranks}}"/>
	<input type="hidden" name="i" value="{{nextrank}}"/>
	<input type="hidden" name="s" value="{{scheme}}"/>
	<input id="navigation_prev" type="image" src="static/arrow-up.png" tabindex="2"/>
	</form>
  {% end %}
  {% if nofranks == len(results) %}
	{% set nextrank = firstrank + nofranks %}

	<form name="nav_next" class method="GET" action="query">
	<input type="hidden" name="q" value="{{querystr}}"/>
	<input type="hidden" name="n" value="{{nofranks}}"/>
	<input type="hidden" name="i" value="{{nextrank}}"/>
	<input type="hidden" name="s" value="{{scheme}}"/>
	<input id="navigation_next" type="image" src="static/arrow-down.png" tabindex="2"/>
	</form>
  {% end %}
  </div>
  </div>
  {% end %}
</body>
</html>

