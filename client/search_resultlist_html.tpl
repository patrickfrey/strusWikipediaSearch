{% extends "search_base_html.tpl" %}

{% block navigation %}
<div id="navigation">
<div id="logo">
 <a href="http://project-strus.net">
  <img width="100%" src="static/strus_logo.jpg" alt="strus logo"/>
<!-- Copyright: <a href='http://www.123rf.com/profile_guarding123'>guarding123 / 123RF Stock Photo</a>
-->
 </a>
</div>

<div id="toolbar">
<form id="searchbox" name="search" class method="GET" action="query">
<select name="s" id="scheme">
 {% if scheme == "NBLNK" %}<option selected>NBLNK</option>{% else %}<option>NBLNK</option>{% end %}
 {% if scheme == "BM25" %}<option selected>BM25</option>{% else %}<option>BM25</option>{% end %}
 {% if scheme == "BM25pff" %}<option selected>BM25pff</option>{% else %}<option>BM25pff</option>{% end %}
</select>
<input id="search" class="textinput" type="text" maxlength="256" size="32" name="q" tabindex="0" value="{{ querystr }}"/>
<input id="submit" type="submit" value="Search" />
<input type="hidden" name="n" value="{{ maxnofranks }}"/>
{% if mode != None %}
 <input type="hidden" name="m" value="{{ mode }}"/>
{% end %}
<div id="DidYouMean">
<div id="DidYouMeanList"></div>
</div>
</form>

{% set prevrank = firstrank - maxnofranks %}
{% if prevrank < 0 %}
{% set prevrank = 0 %}
{% end %}
{% if prevrank < firstrank %}
 <form id="navprev" name="prev" class method="GET" action="query">
 <input type="hidden" name="s" value="{{ scheme }}"/>
 <input type="hidden" name="q" value="{{ querystr }}"/>
 <input id="submit" type="submit" value="<<" />
 <input type="hidden" name="i" value="{{ prevrank }}"/>
 <input type="hidden" name="n" value="{{ maxnofranks }}"/>
{% if mode != None %}
 <input type="hidden" name="m" value="{{ mode }}"/>
{% end %}
</form>
{% end %}
{% if len(results) == maxnofranks %}
{% set nextrank = firstrank + maxnofranks %}
 <form id="navnext" name="next" class method="GET" action="query">
 <input type="hidden" name="s" value="{{ scheme }}"/>
 <input type="hidden" name="q" value="{{ querystr }}"/>
 <input id="submit" type="submit" value=">>" />
 <input type="hidden" name="i" value="{{ nextrank }}"/>
 <input type="hidden" name="n" value="{{ maxnofranks }}"/>
{% if mode != None %}
 <input type="hidden" name="m" value="{{ mode }}"/>
{% end %}
 </form>
{% end %}
</div>
</div>
{% end %}

{% block infoblock %}
{% for message in messages %}
<div id="searcherror"><p>Error: {{message}}</p></div>
{% end %}

{% if len(results) >= 1 %}
{% set rank1 = firstrank + 1 %}
{% set rankN = firstrank + len(results) %}
<div id="searchinfo">
<p>Results {{rank1}} .. {{rankN}}&nbsp;&nbsp;&nbsp; Answer time: {{ "%.3f" % time_elapsed}} seconds</p>
</div>
{% end %}
{% end %}

{% block resultblock %}
{% end %}

