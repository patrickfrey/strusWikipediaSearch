{% extends "search_base_html.tpl" %}

{% block script %}
{% end %}

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
<form id="searchbox" name="search" class method="GET" action="evalQuery.php">
<input id="searchtext" class="textinput" type="text" maxlength="256" size="32" name="q" tabindex="0" value="{{ querystr }}" autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false" />
<input id="submit_search" type="submit" value="Search" />
<input type="hidden" name="i" value="0"/>
<input type="hidden" name="n" value="{{ nofranks }}"/>
{% if scheme != None %}
 <input type="hidden" name="s" value="{{ scheme }}"/>
{% end %}
{% if mode != None %}
 <input type="hidden" name="m" value="{{ mode }}"/>
{% end %}
</form>

{% set prevrank = firstrank - nofranks %}
{% if prevrank < 0 %}
{% set prevrank = 0 %}
{% end %}
{% if prevrank < firstrank %}
 <form id="navprev" name="prev" class method="GET" action="evalQuery.php">
 <input id="submit_prev" type="submit" value="<<" />
 <input type="hidden" name="q" value="{{ querystr }}"/>
 <input type="hidden" name="i" value="{{ prevrank }}"/>
 <input type="hidden" name="n" value="{{ nofranks }}"/>
 {% if scheme != None %}
  <input type="hidden" name="s" value="{{ scheme }}"/>
 {% end %}
 {% if mode != None %}
  <input type="hidden" name="m" value="{{ mode }}"/>
 {% end %}
{% else %}
 <form id="navprev" name="prev" class method="GET">
 <input id="submit_prev" type="submit" value="<<" disabled />
{% end %}
</form>

{% if hasmore %}
{% set nextrank = firstrank + nofranks %}
 <form id="navnext" name="next" class method="GET" action="evalQuery.php">
 <input id="submit_next" type="submit" value=">>" />
 <input type="hidden" name="q" value="{{ querystr }}"/>
 <input type="hidden" name="i" value="{{ nextrank }}"/>
 <input type="hidden" name="n" value="{{ nofranks }}"/>
 {% if scheme != None %}
  <input type="hidden" name="s" value="{{ scheme }}"/>
 {% end %}
 {% if mode != None %}
  <input type="hidden" name="m" value="{{ mode }}"/>
 {% end %}
{% else %}
 <form id="navnext" name="next" class method="GET">
 <input id="submit_next" type="submit" value=">>" disabled />
{% end %}
 </form>
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
{% else %}
<div id="searchinfo">
<p>No matching documents&nbsp;&nbsp;&nbsp; Answer time: {{ "%.3f" % time_elapsed}} seconds</p>
</div>
{% end %}
{% end %}

{% block relatedblock %}{% end %}
{% block nblinksblock %}{% end %}
{% block resultblock %}{% end %}


