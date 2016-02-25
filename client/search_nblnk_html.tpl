{% extends "search_resultlist_html.tpl" %}

{% block resultblock %}
<div id="searchresult">
<ul>
{% for result in results %}
{% set link = result.title.replace(' ','_') %}
<li onclick="parent.location='https://en.wikipedia.org/wiki/{{ link }}'">
<div id="rank">
<div id="rank_linkid">{{ result.title }}</div>
<div id="rank_weight">{{ "%.4f" % result.weight }}</div>
</div>
</li>
{% end %}
</ul>
</div>
{% end %}

